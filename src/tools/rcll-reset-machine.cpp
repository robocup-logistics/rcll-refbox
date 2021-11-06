
/***************************************************************************
 *  rcll-reset-machine.cpp - reset a machine
 *
 *  Created: Mon Mar 04 14:09:00 2013
 *  Copyright  2013-2015  Tim Niemueller [www.niemueller.de]
 *                  2017  Tobias Neumann
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
#include <msgs/BeaconSignal.pb.h>
#include <msgs/GameState.pb.h>
#include <msgs/MachineInfo.pb.h>
#include <msgs/MachineInstructions.pb.h>
#include <protobuf_comm/peer.h>
#include <utils/system/argparser.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool            quit = false;
std::string            machine_name_;
std::string            machine_rotation_;
llsf_msgs::MachineSide bs_side_;
llsf_msgs::BaseColor   bs_color_;
int                    ds_gate_;
llsf_msgs::RingColor   rs_ring_color_;
llsf_msgs::CSOp        cs_operation_;
std::string            team_name_;
Team                   team_color_;
ProtobufBroadcastPeer *peer_public_  = NULL;
ProtobufBroadcastPeer *peer_team_    = NULL;
bool                   crypto_setup_ = false;

llsfrb::Configuration *config_;

void
signal_handler(const boost::system::error_code &error, int signum)
{
	if (!error) {
		quit = true;
	}
}

void
handle_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg)
{
	//printf("Receive error from %s:%u: %s\n",
	//	 endpoint.address().to_string().c_str(), endpoint.port(), msg.c_str());
}

void
handle_send_error(std::string msg)
{
	printf("Send error: %s\n", msg.c_str());
}

void
handle_message(boost::asio::ip::udp::endpoint &           sender,
               uint16_t                                   component_id,
               uint16_t                                   msg_type,
               std::shared_ptr<google::protobuf::Message> msg)
{
	std::shared_ptr<GameState> gs;
	if ((gs = std::dynamic_pointer_cast<GameState>(msg))) {
		int hour = gs->game_time().sec() / 3600;
		int min  = (gs->game_time().sec() - hour * 3600) / 60;
		int sec  = gs->game_time().sec() - hour * 3600 - min * 60;

#if __WORDSIZE == 64
		printf("GameState received:  %02i:%02i:%02i.%02ld  %s %s  %u:%u points, %s vs. %s\n",
#else
		printf("GameState received:  %02i:%02i:%02i.%02lld  %s %s  %u:%u points, %s vs. %s\n",
#endif
		       hour,
		       min,
		       sec,
		       gs->game_time().nsec() / 1000000,
		       llsf_msgs::GameState::Phase_Name(gs->phase()).c_str(),
		       llsf_msgs::GameState::State_Name(gs->state()).c_str(),
		       gs->points_cyan(),
		       gs->points_magenta(),
		       gs->team_cyan().c_str(),
		       gs->team_magenta().c_str());

		if (team_name_ == gs->team_cyan() || team_name_ == gs->team_magenta()) {
			if (team_name_ == gs->team_cyan() && team_color_ != CYAN) {
				printf("WARNING: sending as magenta, but our team is announced as cyan by refbox!\n");
			} else if (team_name_ == gs->team_magenta() && team_color_ != MAGENTA) {
				printf("WARNING: sending as cyan, but our team is announced as magenta by refbox!\n");
			}
			if (!crypto_setup_) {
				crypto_setup_ = true;

				std::string crypto_key = "", cipher = "aes-128-cbc";
				try {
					crypto_key = config_->get_string(("/llsfrb/game/crypto-keys/" + team_name_).c_str());
					printf("Set crypto key to %s (cipher %s)\n", crypto_key.c_str(), cipher.c_str());
					peer_team_->setup_crypto(crypto_key, cipher);
				} catch (Exception &e) {
					printf("No encryption key configured for team, not enabling crypto");
				}
			}
		} else if (crypto_setup_) {
			printf("Our team is not set, training game? Disabling crypto.\n");
			crypto_setup_ = false;
			peer_team_->setup_crypto("", "");
		}
	}

	std::shared_ptr<MachineInfo> mi;
	if ((mi = std::dynamic_pointer_cast<MachineInfo>(msg))) {
		printf("MachineInfo received:\n");
		for (int i = 0; i < mi->machines_size(); ++i) {
			const Machine &m = mi->machines(i);
			if (0 == machine_name_.compare(m.name()) && m.state() == "BROKEN") {
				raise(SIGINT);
				quit = true;
			}
		}
	}

	std::shared_ptr<BeaconSignal> b;
	if ((b = std::dynamic_pointer_cast<BeaconSignal>(msg))) {
		if (b->team_name() == "LLSF" && b->peer_name() == "RefBox") {
			printf("Reset machine\n");
			llsf_msgs::ResetMachine reset_msg;
			reset_msg.set_team_color(team_color_);
			reset_msg.set_machine(machine_name_);
			peer_team_->send(reset_msg);
		}
	}
}

void
usage(const char *progname)
{
	printf("Usage: %s <team-name> <machine-name>\n", progname);
}

int
main(int argc, char **argv)
{
	ArgumentParser argp(argc, argv, "");

	if (argp.num_items() < 2) {
		usage(argv[0]);
		exit(1);
	}

	team_name_    = argp.items()[0];
	machine_name_ = argp.items()[1];

	std::string team_str = machine_name_.substr(0, 1);
	team_color_          = CYAN;
	if (team_str == "C") {
		team_color_ = CYAN;
	} else if (team_str == "M") {
		team_color_ = MAGENTA;
	} else {
		printf("Unknonw team value\n");
		usage(argv[0]);
		exit(-1);
	}

	config_ = new llsfrb::YamlConfiguration(CONFDIR);
	config_->load("config_generated.yaml");

	if (config_->exists("/llsfrb/comm/public-peer/send-port")
	    && config_->exists("/llsfrb/comm/public-peer/recv-port")) {
		peer_public_ =
		  new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
		                            config_->get_uint("/llsfrb/comm/public-peer/recv-port"),
		                            config_->get_uint("/llsfrb/comm/public-peer/send-port"));
	} else {
		peer_public_ = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
		                                         config_->get_uint("/llsfrb/comm/public-peer/port"));
	}

	MessageRegister &message_register = peer_public_->message_register();
	message_register.add_message_type<BeaconSignal>();
	message_register.add_message_type<GameState>();
	message_register.add_message_type<MachineInfo>();

	std::string cfg_prefix =
	  std::string("/llsfrb/comm/") + ((team_color_ == CYAN) ? "cyan" : "magenta") + "-peer/";

	if (config_->exists((cfg_prefix + "send-port").c_str())
	    && config_->exists((cfg_prefix + "recv-port").c_str())) {
		peer_team_ = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
		                                       config_->get_uint((cfg_prefix + "recv-port").c_str()),
		                                       config_->get_uint((cfg_prefix + "send-port").c_str()),
		                                       &message_register /*, crypto_key, cipher*/);
	} else {
		peer_team_ = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
		                                       config_->get_uint((cfg_prefix + "port").c_str()),
		                                       &message_register /*, crypto_key, cipher*/);
	}

	boost::asio::io_service io_service;

	printf("Waiting for beacon from refbox...\n");

	peer_public_->signal_received().connect(handle_message);
	peer_public_->signal_recv_error().connect(handle_recv_error);
	peer_public_->signal_send_error().connect(handle_send_error);

	peer_team_->signal_received().connect(handle_message);
	peer_team_->signal_recv_error().connect(handle_recv_error);
	peer_team_->signal_send_error().connect(handle_send_error);

#if BOOST_ASIO_VERSION >= 100601
	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(signal_handler);
#endif

	do {
		io_service.run();
		io_service.reset();
	} while (!quit);

	delete peer_public_;
	delete peer_team_;
	delete config_;

	// Delete all global objects allocated by libprotobuf
	google::protobuf::ShutdownProtobufLibrary();
}
