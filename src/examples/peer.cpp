
/***************************************************************************
 *  peer.cpp - LLSF RefBox communication - example peer
 *
 *  Created: Fri Mar 08 10:10:16 2013
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

#include <protobuf_comm/peer.h>
#include <msgs/GameState.pb.h>

using namespace protobuf_comm;

#define TEAM_NAME      "Carologistics"
#define CRYPTO_KEY     "randomkey"
#define CRYPTO_CIPHER  "aes-128-cbc"

class ExamplePeer
{
 public:
  ExamplePeer(std::string host, unsigned short port)
    : host_(host), mr_(new MessageRegister()), peer_team_(NULL)
  {
    mr_->add_message_type<llsf_msgs::GameState>();
    peer_public_ = setup_peer(port, /* encrypted? */ false);
  }

  ~ExamplePeer() {
    delete peer_team_;
    delete peer_public_;
  }

 private:
  void peer_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg) {
    printf("Receive error from %s:%u: %s\n", endpoint.address().to_string().c_str(),
	   endpoint.port(), msg.c_str());
  }

  void peer_send_error(std::string msg) {
    printf("Send error: %s\n", msg.c_str());
  }

  void peer_msg(boost::asio::ip::udp::endpoint &endpoint, uint16_t comp_id, uint16_t msg_type,
		std::shared_ptr<google::protobuf::Message> msg)
  {
    std::shared_ptr<llsf_msgs::GameState> g;
    if ((g = std::dynamic_pointer_cast<llsf_msgs::GameState>(msg))) {
      printf("GameState received from %s: %u/%u points\n",
	     endpoint.address().to_string().c_str(),
	     g->points_cyan(), g->points_magenta());

      if (! peer_team_) {
	if (g->team_cyan()    == TEAM_NAME)  peer_team_ = setup_peer(4441, true);
	if (g->team_magenta() == TEAM_NAME)  peer_team_ = setup_peer(4442, true);
      } else {
	if (g->team_cyan() != TEAM_NAME && g->team_magenta() != TEAM_NAME) {
	  delete peer_team_; peer_team_ = NULL;
	}
      }
    }
  }

  ProtobufBroadcastPeer * setup_peer(unsigned short int port, bool crypto) {
    ProtobufBroadcastPeer *peer = crypto
      ? new ProtobufBroadcastPeer(host_, port, mr_, CRYPTO_KEY, CRYPTO_CIPHER)
      : new ProtobufBroadcastPeer(host_, port, mr_);
    peer->signal_recv_error().connect(
      boost::bind(&ExamplePeer::peer_recv_error, this, _1, _2));
    peer->signal_send_error().connect(
      boost::bind(&ExamplePeer::peer_send_error, this, _1));
    peer->signal_received().connect(
      boost::bind(&ExamplePeer::peer_msg, this, _1, _2, _3, _4));
    return peer;
  }

 private:
  std::string            host_;
  MessageRegister       *mr_;
  ProtobufBroadcastPeer *peer_public_;
  ProtobufBroadcastPeer *peer_team_;
};

int main(int argc, char **argv) {
  ExamplePeer peer("137.226.233.255", 4444);
  while (true) usleep(100000);
}
