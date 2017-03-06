
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

#include <protobuf_comm/client.h>
#include <utils/system/argparser.h>

#include <msgs/GameState.pb.h>
#include <msgs/GameInfo.pb.h>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

boost::asio::io_service io_service_;

static bool quit = false;
ProtobufStreamClient *client_ = NULL;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
    io_service_.stop();
  }
}

void
handle_connected(llsf_msgs::SetTeamName *msg_team_cyan,
                 llsf_msgs::SetTeamName *msg_team_magenta,
                 llsf_msgs::SetGamePhase *msg_phase,
                 llsf_msgs::SetGameState *msg_state)
{
  if (msg_team_cyan) {
	  printf("Sending cyan team: %s\n", msg_team_cyan->team_name().c_str());
	  client_->send(*msg_team_cyan);
  }
  if (msg_team_magenta) {
	  printf("Sending magenta team: %s\n", msg_team_magenta->team_name().c_str());
	  client_->send(*msg_team_magenta);
  }
  if (msg_phase) {
	  printf("Sending Phase: %s\n", GameState_Phase_Name(msg_phase->phase()).c_str());
	  client_->send(*msg_phase);
  }
  if (msg_state) {
	  printf("Sending State: %s\n", GameState_State_Name(msg_state->state()).c_str());
	  client_->send(*msg_state);
  }
  quit = true;
  io_service_.stop();
}

void
handle_disconnected(const boost::system::error_code &ec)
{
  quit = true;
  io_service_.stop();
}

void
handle_message(uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
}



void
usage(const char *progname)
{
  printf("Usage: %s [options]\n\n"
         "Valid options are:\n",
         progname);
  
  printf(" -p <phase>       Set game phase\n"
         "                  Phase is one of:\n");
  const ::google::protobuf::EnumDescriptor* phase_desc =
	  GameState_Phase_descriptor();
  for (int i = 0; i < phase_desc->value_count(); ++i) {
	  printf("                  - %s\n", phase_desc->value(i)->name().c_str());
  }

  printf(" -s <state>       Set game state\n"
         "                  State is one of:\n");
  const ::google::protobuf::EnumDescriptor* state_desc =
	  GameState_State_descriptor();
  for (int i = 0; i < state_desc->value_count(); ++i) {
	  printf("                  - %s\n", state_desc->value(i)->name().c_str());
  }
  
  printf(" -c <team name>   Set name of Cyan team\n"
         " -m <team name>   Set name of Magenta team\n"
         " -r <remote>      Connect to given host\n"
         "                  remote is of the form: host[:port]\n"
         " -h               Show this help message\n");
}


int
main(int argc, char **argv)
{
  client_ = new ProtobufStreamClient();

  ArgumentParser argp(argc, argv, "hs:p:c:m:r:");

  if (argp.has_arg("h")) {
    usage(argv[0]);
    exit(1);
  }

  llsf_msgs::SetTeamName *msg_team_cyan = NULL, *msg_team_magenta = NULL;
  llsf_msgs::SetGamePhase *msg_phase = NULL;
  llsf_msgs::SetGameState *msg_state = NULL;

  // Parse parameters and messages here for early failure

  if (argp.has_arg("p")) {
	  llsf_msgs::GameState::Phase p;
	  if (! llsf_msgs::GameState::Phase_Parse(argp.arg("p"), &p)) {
		  printf("Invalid phase '%s'\n\n", argp.arg("p"));
		  usage(argv[0]);
		  exit(2);
	  }
	  msg_phase = new llsf_msgs::SetGamePhase();
	  msg_phase->set_phase(p);
  }
  if (argp.has_arg("s")) {
	  llsf_msgs::GameState::State s;
	  if (! llsf_msgs::GameState::State_Parse(argp.arg("s"), &s)) {
		  printf("Invalid state '%s'\n\n", argp.arg("s"));
		  usage(argv[0]);
		  exit(2);
	  }
	  msg_state = new llsf_msgs::SetGameState();
	  msg_state->set_state(s);
  }
  if (argp.has_arg("c")) {
	  msg_team_cyan = new llsf_msgs::SetTeamName();
	  msg_team_cyan->set_team_name(argp.arg("c"));
	  msg_team_cyan->set_team_color(llsf_msgs::CYAN);
  }
  if (argp.has_arg("m")) {
	  msg_team_magenta = new llsf_msgs::SetTeamName();
	  msg_team_magenta->set_team_name(argp.arg("m"));
	  msg_team_magenta->set_team_color(llsf_msgs::MAGENTA);
  }

  //MessageRegister & message_register = client_->message_register();

  client_->signal_received().connect(handle_message);
  client_->signal_connected().connect(boost::bind(handle_connected,
                                                  msg_team_cyan, msg_team_magenta,
                                                  msg_phase, msg_state));
  client_->signal_disconnected().connect(handle_disconnected);
  client_->async_connect("localhost", 4444);

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
  } while (! quit);

  unsigned int tries = 0;
  while (! client_->outbound_done() && ++tries < 500) {
	  usleep(20000);
  }
  if (! client_->outbound_done()) {
	  printf("Failed to send some outbound messages\n");
  }
  delete client_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
