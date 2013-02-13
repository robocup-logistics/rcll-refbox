
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
handle_error(const boost::system::error_code &error)
{
  printf("Error: %s\n", error.message().c_str());
}

void
handle_message(boost::asio::ip::udp::endpoint &sender,
	       uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<BeaconSignal> b;
  if ((b = std::dynamic_pointer_cast<BeaconSignal>(msg))) {
    printf("Detected robot: %s:%s (seq %lu)\n",
	   b->team_name().c_str(), b->peer_name().c_str(), b->seq());
  }
}

int
main(int argc, char **argv)
{
  llsfrb::Configuration *config = new llsfrb::YamlConfiguration(CONFDIR);
  config->load("config.yaml");

  ProtobufBroadcastPeer *peer =
    new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
			      config->get_uint("/llsfrb/comm/peer-port"));

  boost::asio::io_service io_service;

  MessageRegister & message_register = peer->message_register();
  message_register.add_message_type<BeaconSignal>();

  peer->signal_received().connect(handle_message);
  peer->signal_error().connect(handle_error);

  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);

  do {
    io_service.run();
    io_service.reset();
  } while (! quit);

  delete peer;
  delete config;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
