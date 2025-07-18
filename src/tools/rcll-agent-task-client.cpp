// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  rcll-agent-task-client.cpp - send agent tasks and wait for responses
 *
 *  Created: Thu Jul 17 2025
 *  Copyright  2025  Tim Wendt
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config/yaml.h>
#include <msgs/AgentTask.pb.h>
#include <msgs/BeaconSignal.pb.h>
#include <msgs/GameState.pb.h>
#include <protobuf_comm/peer.h>
#include <utils/system/argparser.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <chrono>
#include <iostream>
#include <memory>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool                              quit = false;
std::string                              team_name_;
Team                                     team_color_;
std::string                              crypto_key_;
uint32_t                                 robot_id_;
uint32_t                                 task_id_;
std::string                              operation_;
std::string                              target_;
std::string                              poi_;
ProtobufBroadcastPeer                   *peer_public_  = NULL;
ProtobufBroadcastPeer                   *peer_team_    = NULL;
ProtobufBroadcastPeer                   *peer_robot_   = NULL;
bool                                     crypto_setup_ = false;
bool                                     task_sent_ = false;
bool                                     task_response_received_ = false;
boost::asio::io_service                  io_service;
boost::asio::deadline_timer              timer(io_service);

rcll::Configuration *config_;

void
signal_handler(const boost::system::error_code &error, int signum)
{
	timer.cancel();
	io_service.stop();
	if (!error) {
		quit = true;
	}
}

void
handle_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg)
{
	printf("Receive error from %s:%u: %s\n",
	       endpoint.address().to_string().c_str(),
	       endpoint.port(),
	       msg.c_str());
}

void
handle_send_error(std::string msg)
{
	printf("Send error: %s\n", msg.c_str());
}

void
send_agent_task()
{
	llsf_msgs::AgentTask msg;
	msg.set_team_color(team_color_);
	msg.set_task_id(task_id_);
	msg.set_robot_id(robot_id_);

	if (operation_ == "MOVE") {
		llsf_msgs::Move *move = new llsf_msgs::Move();
		move->set_waypoint(target_);
		if (!poi_.empty()) {
			move->set_machine_point(poi_);
		}
		msg.set_allocated_move(move);
	} else if (operation_ == "DELIVER") {
		llsf_msgs::Deliver *deliver = new llsf_msgs::Deliver();
		deliver->set_machine_id(target_);
		deliver->set_machine_point(poi_);
		msg.set_allocated_deliver(deliver);
	} else if (operation_ == "RETRIEVE") {
		llsf_msgs::Retrieve *retrieve = new llsf_msgs::Retrieve();
		retrieve->set_machine_id(target_);
		retrieve->set_machine_point(poi_);
		msg.set_allocated_retrieve(retrieve);
	} else if (operation_ == "BUFFER") {
		llsf_msgs::BufferStation *buffer = new llsf_msgs::BufferStation();
		buffer->set_machine_id(target_);
		if (!poi_.empty()) {
			buffer->set_shelf_number(std::stoi(poi_));
		}
		msg.set_allocated_buffer(buffer);
	} else if (operation_ == "EXPLORE") {
		llsf_msgs::ExploreWaypoint *explore = new llsf_msgs::ExploreWaypoint();
		explore->set_machine_id(target_);
		if (!poi_.empty()) {
			explore->set_machine_point(poi_);
		}
		explore->set_waypoint(target_);
		msg.set_allocated_explore_machine(explore);
	} else {
		printf("Unknown operation %s, expected one of (MOVE | DELIVER | RETRIEVE | BUFFER | EXPLORE)\n",
		       operation_.c_str());
		return;
	}

	printf("Sending %s task (ID: %u, Robot: %u, Team: %s) to %s %s\n", 
	       operation_.c_str(), task_id_, robot_id_, 
	       (team_color_ == Team::CYAN) ? "CYAN" : "MAGENTA", 
	       target_.c_str(), poi_.c_str());
	
	if (peer_robot_) {
		printf("Using robot-specific communication\n");
		peer_robot_->send(msg);
	} else if (peer_team_) {
		printf("Using encrypted team communication\n");
		peer_team_->send(msg);
	} else if (peer_public_) {
		printf("Using public communication (no encryption setup yet)\n");
		peer_public_->send(msg);
	}
	
	task_sent_ = true;
}

void
handle_message(boost::asio::ip::udp::endpoint            &sender,
               uint16_t                                   component_id,
               uint16_t                                   msg_type,
               std::shared_ptr<google::protobuf::Message> msg)
{
	std::shared_ptr<GameState> gs;
	std::shared_ptr<AgentTask> at;

	if ((gs = std::dynamic_pointer_cast<GameState>(msg))) {
		printf("Received GameState message\n");
		
		if (team_name_ == gs->team_cyan() || team_name_ == gs->team_magenta()) {
			if (team_name_ == gs->team_cyan() && team_color_ != CYAN) {
				printf("WARNING: sending as magenta, but our team is announced as cyan by refbox!\n");
			} else if (team_name_ == gs->team_magenta() && team_color_ != MAGENTA) {
				printf("WARNING: sending as cyan, but our team is announced as magenta by refbox!\n");
			}
			if (!crypto_setup_) {
				crypto_setup_ = true;
				printf("Set crypto key to %s (cipher aes-128-cbc)\n", crypto_key_.c_str());
				peer_team_->setup_crypto(crypto_key_, "aes-128-cbc");
			}
		} else if (crypto_setup_) {
			printf("Our team is not set, training game? Disabling crypto.\n");
			crypto_setup_ = false;
			peer_team_->setup_crypto("", "");
		}
	}

	if ((at = std::dynamic_pointer_cast<AgentTask>(msg))) {
		printf("Received AgentTask message - Task ID: %u, Robot ID: %u\n", 
		       at->task_id(), at->robot_id());
		
		// Check if this is the response to the sent task
		if (task_sent_ && at->task_id() == task_id_ && at->robot_id() == robot_id_) {
			printf("Received response for the task!\n");
			
			if (at->has_successful()) {
				printf("Task status: %s\n", at->successful() ? "SUCCESSFUL" : "FAILED");
			}
			if (at->has_canceled()) {
				printf("Task canceled: %s\n", at->canceled() ? "YES" : "NO");
			}
			if (at->has_error_code()) {
				printf("Error code: %u\n", at->error_code());
			}
			
			task_response_received_ = true;
			quit = true;
		}
	}
}

void
print_usage(const char *program_name)
{
	printf("Usage: %s <team_name> <robot_id> <task_id> <operation> <target> [poi]\n", program_name);
	printf("  team_name:    Name of your team (must be configured)\n");
	printf("  robot_id:     ID of the robot (1-3)\n");
	printf("  task_id:      Unique task identifier\n");
	printf("  operation:    MOVE, DELIVER, RETRIEVE, BUFFER, or EXPLORE\n");
	printf("  target:       Zone/machine name (e.g., C-RS1, C_Z32)\n");
	printf("  poi:          Point of interest (INPUT, OUTPUT, SHELF, or shelf number)\n");
	printf("\nExamples:\n");
	printf("  %s Carologistics 1 100 MOVE C-RS1 INPUT\n", program_name);
	printf("  %s Carologistics 2 101 RETRIEVE C-RS1 OUTPUT\n", program_name);
	printf("  %s Carologistics 3 102 DELIVER C-CS1 INPUT\n", program_name);
}

int
main(int argc, char **argv)
{
	ArgumentParser argp(argc, argv, "T:");

	if (argp.num_items() < 5) {
		print_usage(argv[0]);
		exit(1);
	}

	team_name_ = argp.items()[0];
	robot_id_ = argp.parse_item_int(1);
	task_id_ = argp.parse_item_int(2);
	operation_ = argp.items()[3];
	target_ = argp.items()[4];
	if (argp.num_items() > 5) {
		poi_ = argp.items()[5];
	}

	// Determine team color from target name
	std::string team_str = target_.substr(0, 1);
	team_color_ = CYAN;
	if (team_str == "C") {
		team_color_ = CYAN;
	} else if (team_str == "M") {
		team_color_ = MAGENTA;
	} else {
		printf("Unknown team value from target, using cyan\n");
	}

	config_ = new rcll::YamlConfiguration(CONFDIR);
	config_->load("config_generated.yaml");

	// Get crypto key for the team
	try {
		crypto_key_ = config_->get_string(("/llsfrb/game/crypto-keys/" + team_name_).c_str());
		printf("Found crypto key for team %s\n", team_name_.c_str());
	} catch (fawkes::Exception &e) {
		printf("No encryption key configured for team %s\n", team_name_.c_str());
	}

	// Setup message register
	std::vector<std::string> proto_dirs;
	try {
		std::vector<std::string> proto_dirs_config = config_->get_strings("/llsfrb/comm/protobuf-dirs");
		for (auto &d : proto_dirs_config) {
			proto_dirs.push_back(d);
		}
	} catch (fawkes::Exception &e) {
		proto_dirs.push_back(std::string(SHAREDIR) + "/msgs");
	}

	MessageRegister message_register(proto_dirs);
	message_register.add_message_type<llsf_msgs::AgentTask>();
	message_register.add_message_type<llsf_msgs::GameState>();
	message_register.add_message_type<llsf_msgs::BeaconSignal>();

	// Setup public peer
	if (config_->exists("/llsfrb/comm/public-peer/send-port")
	    && config_->exists("/llsfrb/comm/public-peer/recv-port")) {
		peer_public_ =
		  new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
		                            config_->get_uint("/llsfrb/comm/public-peer/recv-port"),
		                            config_->get_uint("/llsfrb/comm/public-peer/send-port"),
		                            &message_register);
	} else {
		peer_public_ = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
		                                         config_->get_uint("/llsfrb/comm/public-peer/port"),
		                                         &message_register);
	}

	// Setup team peer
	std::string cfg_prefix =
	  std::string("/llsfrb/comm/") + ((team_color_ == CYAN) ? "cyan" : "magenta") + "-peer/";

	if (config_->exists((cfg_prefix + "send-port").c_str())
	    && config_->exists((cfg_prefix + "recv-port").c_str())) {
		peer_team_ = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
		                                       config_->get_uint((cfg_prefix + "recv-port").c_str()),
		                                       config_->get_uint((cfg_prefix + "send-port").c_str()),
		                                       &message_register);
	} else {
		peer_team_ = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
		                                       config_->get_uint((cfg_prefix + "port").c_str()),
		                                       &message_register);
	}

	// Setup robot-specific peer if configured
	std::string robot_cfg_prefix = 
	  std::string("/llsfrb/robot-comm/") + team_name_ + "/robot-" + std::to_string(robot_id_) + "/";
	
	try {
		if (config_->exists((robot_cfg_prefix + "send-port").c_str())
		    && config_->exists((robot_cfg_prefix + "recv-port").c_str())) {
			std::string host = config_->get_string("/llsfrb/comm/public-peer/host");
			unsigned int send_port = config_->get_uint((robot_cfg_prefix + "recv-port").c_str());  // Send to robot's recv port
			unsigned int recv_port = config_->get_uint((robot_cfg_prefix + "send-port").c_str());  // Listen on robot's send port
			
			printf("Setting up robot-specific communication on %s send:%u recv:%u\n", 
			       host.c_str(), send_port, recv_port);
			
			peer_robot_ = new ProtobufBroadcastPeer(host, recv_port, send_port, &message_register);
			peer_robot_->signal_received().connect(boost::bind(&handle_message, 
			                                                   boost::placeholders::_1, 
			                                                   boost::placeholders::_2,
			                                                   boost::placeholders::_3, 
			                                                   boost::placeholders::_4));
			peer_robot_->signal_recv_error().connect(boost::bind(&handle_recv_error, 
			                                                     boost::placeholders::_1, 
			                                                     boost::placeholders::_2));
			peer_robot_->signal_send_error().connect(boost::bind(&handle_send_error, 
			                                                     boost::placeholders::_1));
		}
	} catch (fawkes::Exception &e) {
		printf("No robot-specific ports configured for %s robot %u, using team communication\n", 
		       team_name_.c_str(), robot_id_);
	}

	peer_public_->signal_received().connect(boost::bind(&handle_message, 
	                                                   boost::placeholders::_1, 
	                                                   boost::placeholders::_2,
	                                                   boost::placeholders::_3, 
	                                                   boost::placeholders::_4));
	peer_public_->signal_recv_error().connect(boost::bind(&handle_recv_error, 
	                                                     boost::placeholders::_1, 
	                                                     boost::placeholders::_2));
	peer_public_->signal_send_error().connect(boost::bind(&handle_send_error, 
	                                                     boost::placeholders::_1));

	peer_team_->signal_received().connect(boost::bind(&handle_message, 
	                                                 boost::placeholders::_1, 
	                                                 boost::placeholders::_2,
	                                                 boost::placeholders::_3, 
	                                                 boost::placeholders::_4));
	peer_team_->signal_recv_error().connect(boost::bind(&handle_recv_error, 
	                                                   boost::placeholders::_1, 
	                                                   boost::placeholders::_2));
	peer_team_->signal_send_error().connect(boost::bind(&handle_send_error, 
	                                                   boost::placeholders::_1));

	// Setup signal handlers
	boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(signal_handler);

	// Set a timeout
	timer.expires_from_now(boost::posix_time::seconds(30));
	timer.async_wait([&](const boost::system::error_code &error) {
		if (!error && !task_response_received_) {
			printf("Timeout waiting for task response\n");
			quit = true;
			io_service.stop();
		}
	});

	// Send the agent task repeatedly until we get a response
	auto send_timer = std::make_shared<boost::asio::deadline_timer>(io_service);
	std::function<void()> send_task_repeatedly = [&, send_timer]() {
		if (!task_response_received_ && !quit) {
			send_agent_task();
			send_timer->expires_from_now(boost::posix_time::seconds(3));
			send_timer->async_wait([&](const boost::system::error_code &error) {
				if (!error && !task_response_received_ && !quit) {
					send_task_repeatedly();
				}
			});
		}
	};
	
	send_timer->expires_from_now(boost::posix_time::seconds(1));
	send_timer->async_wait([&](const boost::system::error_code &error) {
		if (!error) {
			send_task_repeatedly();
		}
	});

	printf("Starting message loop. Press Ctrl+C to quit.\n");

	// Main loop
	while (!quit) {
		try {
			io_service.run_one();
		} catch (std::exception &e) {
			printf("Exception: %s\n", e.what());
			break;
		}
	}

	printf("Shutting down...\n");

	if (peer_public_) delete peer_public_;
	if (peer_team_) delete peer_team_;
	if (peer_robot_) delete peer_robot_;
	delete config_;

	// Delete all global objects allocated by libprotobuf
	google::protobuf::ShutdownProtobufLibrary();
	return task_response_received_ ? 0 : 1;
}
