
/***************************************************************************
 *  rcll-refbox-instruct - instruct referee box remotely
 *
 *  Created: Tue Feb 28 15:12:29 2017
 *  Copyright  2013-2017  Tim Niemueller [www.niemueller.de]
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

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include <msgs/GameInfo.pb.h>
#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/VersionInfo.pb.h>
#include <protobuf_comm/client.h>
#include <utils/system/argparser.h>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static boost::asio::io_service io_service_;
boost::asio::deadline_timer    reconnect_timer_(io_service_);

static bool                 quit_             = false;
static bool                 wait_refbox_      = false;
static unsigned int         wait_refbox_time_ = 0;
boost::asio::deadline_timer wait_refbox_timer_(io_service_);
static bool                 wait_state_      = false;
static unsigned int         wait_state_time_ = 0;
boost::asio::deadline_timer wait_state_timer_(io_service_);
ProtobufStreamClient *      client_ = NULL;
static std::string          host_   = "localhost";
static unsigned short int   port_   = 4444;

llsf_msgs::SetTeamName * msg_team_cyan_ = NULL, *msg_team_magenta_ = NULL;
llsf_msgs::SetGamePhase *msg_phase_ = NULL;
llsf_msgs::SetGameState *msg_state_ = NULL;

std::shared_ptr<llsf_msgs::GameState> current_game_state_ = nullptr;
std::shared_ptr<llsf_msgs::RobotInfo> current_robot_info_ = nullptr;

int num_bots_ = -1;

boost::posix_time::ptime start_time_(boost::posix_time::second_clock::universal_time());

int exitcode_ = 0;

void
quit(int exitcode = 0, const char *errmsg = NULL)
{
	if (errmsg)
		fprintf(stderr, "%s\n", errmsg);
	exitcode_ = exitcode;
	wait_refbox_timer_.cancel();
	quit_ = true;
	io_service_.stop();
}

void
signal_handler(const boost::system::error_code &error, int signum)
{
	if (!error)
		quit();
}

void
handle_disconnected(const boost::system::error_code &ec)
{
	if (wait_refbox_) {
		reconnect_timer_.expires_from_now(boost::posix_time::seconds(1));
		reconnect_timer_.async_wait(
		  boost::bind(&ProtobufStreamClient::async_connect, client_, host_.c_str(), port_));
	} else {
		fprintf(stderr, "Failed to connect: %s\n", ec.message().c_str());
		quit(1);
	}
}

void
handle_message(uint16_t                                   component_id,
               uint16_t                                   msg_type,
               std::shared_ptr<google::protobuf::Message> msg)
{
	std::shared_ptr<VersionInfo> v;
	if ((v = std::dynamic_pointer_cast<VersionInfo>(msg))) {
		wait_refbox_timer_.cancel();

		if (wait_state_) {
			if (wait_state_time_ > 0) {
				wait_refbox_timer_.expires_from_now(boost::posix_time::seconds(wait_state_time_));
				wait_refbox_timer_.async_wait([](const boost::system::error_code &ec) {
					if (!ec)
						quit(2, "Timeout waiting for state");
				});
			}
		} else {
			// connected, send what we came for
			if (msg_team_cyan_) {
				printf("Sending cyan team: %s\n", msg_team_cyan_->team_name().c_str());
				client_->send(*msg_team_cyan_);
			}
			if (msg_team_magenta_) {
				printf("Sending magenta team: %s\n", msg_team_magenta_->team_name().c_str());
				client_->send(*msg_team_magenta_);
			}
			if (msg_phase_) {
				printf("Sending Phase: %s\n", GameState_Phase_Name(msg_phase_->phase()).c_str());
				client_->send(*msg_phase_);
			}
			if (msg_state_) {
				printf("Sending State: %s\n", GameState_State_Name(msg_state_->state()).c_str());
				client_->send(*msg_state_);
			}
			quit();
		}
	}

	std::shared_ptr<GameState> g;
	if ((g = std::dynamic_pointer_cast<GameState>(msg))) {
		current_game_state_ = g;
	}
	std::shared_ptr<RobotInfo> ri;
	if ((ri = std::dynamic_pointer_cast<RobotInfo>(msg)) && wait_state_) {
		current_robot_info_ = ri;
	}
	if (wait_state_ && current_game_state_ && current_robot_info_) {
		bool matches = true;
		if (msg_team_cyan_ && msg_team_cyan_->team_name() != current_game_state_->team_cyan()) {
			matches = false;
		}
		if (msg_team_magenta_
		    && msg_team_magenta_->team_name() != current_game_state_->team_magenta()) {
			matches = false;
		}
		if (msg_phase_ && msg_phase_->phase() != current_game_state_->phase()) {
			matches = false;
		}
		if (msg_state_ && msg_state_->state() != current_game_state_->state()) {
			matches = false;
		}
		if (num_bots_ > 0 && current_robot_info_->robots_size() < num_bots_) {
			matches = false;
		}
		if (matches) {
			wait_state_timer_.cancel();
			quit();
		}
	}
}

void
usage(const char *progname)
{
	printf("Usage: %s [options]\n\n"
	       "Valid options are:\n",
	       progname);

	printf(" -p <phase>       Set game phase\n"
	       "                  Phase is one of:\n");
	const ::google::protobuf::EnumDescriptor *phase_desc = GameState_Phase_descriptor();
	for (int i = 0; i < phase_desc->value_count(); ++i) {
		printf("                  - %s\n", phase_desc->value(i)->name().c_str());
	}

	printf(" -s <state>       Set game state\n"
	       "                  State is one of:\n");
	const ::google::protobuf::EnumDescriptor *state_desc = GameState_State_descriptor();
	for (int i = 0; i < state_desc->value_count(); ++i) {
		printf("                  - %s\n", state_desc->value(i)->name().c_str());
	}

	printf(" -c <team name>   Set name of Cyan team\n"
	       " -m <team name>   Set name of Magenta team\n"
	       " -n <number>      Wait for the given number of robots\n"
	       "                  (only useful in conjunction with -W)\n"
	       " -r <remote>      Connect to given host\n"
	       "                  remote is of the form: host[:port]\n"
	       " -w[T]            Wait for refbox startup, optionally wait at most T seconds\n"
	       " -W[T]            Wait for given phase/state/teams/robots,\n"
	       "                  optionally wait at most T seconds\n"
	       " -h               Show this help message\n");
}

int
main(int argc, char **argv)
{
	client_ = new ProtobufStreamClient();

	ArgumentParser argp(argc, argv, "hw::W::s:p:c:m:r:n:");

	if (argp.has_arg("h")) {
		usage(argv[0]);
		exit(1);
	}

	// Parse parameters and messages here for early failure

	if (argp.has_arg("r")) {
		argp.parse_hostport("r", host_, port_);
	}

	if (argp.has_arg("w")) {
		wait_refbox_ = true;
		if (argp.arg("w")) {
			wait_refbox_time_ = argp.parse_int("w");
			wait_refbox_timer_.expires_from_now(boost::posix_time::seconds(wait_refbox_time_));
			wait_refbox_timer_.async_wait([](const boost::system::error_code &ec) {
				if (!ec)
					quit(3, "Timeout waiting for refbox");
			});
		}
	}

	if (argp.has_arg("W")) {
		if (argp.arg("W")) {
			wait_state_time_ = argp.parse_int("W");
		}
		wait_state_ = true;
	}

	if (argp.has_arg("p")) {
		llsf_msgs::GameState::Phase p;
		if (!llsf_msgs::GameState::Phase_Parse(argp.arg("p"), &p)) {
			printf("Invalid phase '%s'\n\n", argp.arg("p"));
			usage(argv[0]);
			exit(2);
		}
		msg_phase_ = new llsf_msgs::SetGamePhase();
		msg_phase_->set_phase(p);
	}
	if (argp.has_arg("s")) {
		llsf_msgs::GameState::State s;
		if (!llsf_msgs::GameState::State_Parse(argp.arg("s"), &s)) {
			printf("Invalid state '%s'\n\n", argp.arg("s"));
			usage(argv[0]);
			exit(2);
		}
		msg_state_ = new llsf_msgs::SetGameState();
		msg_state_->set_state(s);
	}
	if (argp.has_arg("c")) {
		msg_team_cyan_ = new llsf_msgs::SetTeamName();
		msg_team_cyan_->set_team_name(argp.arg("c"));
		msg_team_cyan_->set_team_color(llsf_msgs::CYAN);
	}
	if (argp.has_arg("m")) {
		msg_team_magenta_ = new llsf_msgs::SetTeamName();
		msg_team_magenta_->set_team_name(argp.arg("m"));
		msg_team_magenta_->set_team_color(llsf_msgs::MAGENTA);
	}
	if (argp.has_arg("n")) {
		if (!argp.has_arg("W")) {
			usage(argv[0]);
			exit(2);
		}
		num_bots_ = atoi(argp.arg("n"));
	}

	MessageRegister &message_register = client_->message_register();
	message_register.add_message_type<VersionInfo>();
	message_register.add_message_type<GameState>();
	message_register.add_message_type<RobotInfo>();

	client_->signal_received().connect(handle_message);
	/*
  client_->signal_connected().connect(boost::bind(handle_connected,
                                                  msg_team_cyan, msg_team_magenta,
                                                  msg_phase, msg_state));
  */
	client_->signal_disconnected().connect(handle_disconnected);
	client_->async_connect(host_.c_str(), port_);

#if BOOST_ASIO_VERSION >= 100601
	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(signal_handler);
#endif

	boost::asio::io_service::work io_service_work(io_service_);

	do {
		io_service_.run();
		io_service_.reset();
	} while (!quit_);

	unsigned int tries = 0;
	while (!client_->outbound_done() && ++tries < 500) {
		usleep(20000);
	}
	if (!client_->outbound_done()) {
		printf("Failed to send some outbound messages\n");
	}
	delete client_;

	// Delete all global objects allocated by libprotobuf
	google::protobuf::ShutdownProtobufLibrary();

	return exitcode_;
}
