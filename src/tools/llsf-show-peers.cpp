
/***************************************************************************
 *  llsf-show-peers.cpp - show peers on the network
 *
 *  Created: Tue Feb 12 23:15:26 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
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

#include <protobuf_comm/peer.h>
#include <boost/asio.hpp>
#include <msgs/BeaconSignal.pb.h>
#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>

using namespace protobuf_comm;
using namespace llsf_msgs;

static bool quit = false;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
  }
}

void
handle_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg)
{
  printf("Receive error from %s:%u: %s\n",
	 endpoint.address().to_string().c_str(), endpoint.port(), msg.c_str());
}

void
handle_send_error(std::string msg)
{
  printf("Send error: %s\n", msg.c_str());
}

void
handle_message(boost::asio::ip::udp::endpoint &sender,
	       uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<BeaconSignal> b;
  if ((b = std::dynamic_pointer_cast<BeaconSignal>(msg))) {
#if __WORDSIZE == 64
    printf("Detected robot: %s:%s (seq %lu, team %s)\n",
#else
    printf("Detected robot: %s:%s (seq %llu, team %s)\n",
#endif
	   b->team_name().c_str(), b->peer_name().c_str(), b->seq(),
	   b->has_team_color() ? Team_Name(b->team_color()).c_str() : "NONE");
  }
}

int
main(int argc, char **argv)
{
  llsfrb::Configuration *config = new llsfrb::YamlConfiguration(CONFDIR);
  config->load("config.yaml");

  ProtobufBroadcastPeer *peer;

  if (config->exists("/llsfrb/comm/peer-send-port") &&
      config->exists("/llsfrb/comm/peer-recv-port") )
  {
    peer = new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
				     config->get_uint("/llsfrb/comm/peer-recv-port"),
				     config->get_uint("/llsfrb/comm/peer-send-port"));
  } else {
    peer = new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
				     config->get_uint("/llsfrb/comm/peer-port"));
  }

  boost::asio::io_service io_service;

  MessageRegister & message_register = peer->message_register();
  message_register.add_message_type<BeaconSignal>();
  message_register.add_message_type<GameState>();
  message_register.add_message_type<RobotInfo>();

  peer->signal_received().connect(handle_message);
  peer->signal_recv_error().connect(handle_recv_error);
  peer->signal_send_error().connect(handle_send_error);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  do {
    io_service.run();
    io_service.reset();
  } while (! quit);

  delete peer;
  delete config;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
