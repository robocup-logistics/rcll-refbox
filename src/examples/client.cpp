
/***************************************************************************
 *  client.cpp - LLSF RefBox communication - example client
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

#include <protobuf_comm/client.h>
#include <msgs/GameState.pb.h>

using namespace protobuf_comm;

class ExampleClient
{
 public:
  ExampleClient(std::string host, unsigned short port)
    : host_(host), port_(port)
  {
    client_ = new ProtobufStreamClient();

    MessageRegister & message_register = client_->message_register();
    message_register.add_message_type<llsf_msgs::GameState>();

    client_->signal_connected().connect(
      boost::bind(&ExampleClient::client_connected, this));
    client_->signal_disconnected().connect(
      boost::bind(&ExampleClient::client_disconnected,
		this, boost::asio::placeholders::error));
    client_->signal_received().connect(
      boost::bind(&ExampleClient::client_msg, this, _1, _2, _3));

    client_->async_connect(host.c_str(), port);
  }


  ~ExampleClient()
  {
    delete client_;
  }

 private:
  void client_connected()
  { printf("Client connected\n"); }

  void client_disconnected(const boost::system::error_code &error)
  {
    printf("Client DISconnected\n");
    usleep(100000);
    client_->async_connect(host_.c_str(), port_);
  }

  void client_msg(uint16_t comp_id, uint16_t msg_type,
		  std::shared_ptr<google::protobuf::Message> msg)
  {
    std::shared_ptr<llsf_msgs::GameState> g;
    if ((g = std::dynamic_pointer_cast<llsf_msgs::GameState>(msg))) {
      printf("GameState received: %u/%u points\n",
	     g->points_cyan(), g->points_magenta());
    }
  }

 private:
  ProtobufStreamClient *client_;
  std::string host_;
  unsigned short port_;
};


int main(int argc, char **argv)
{
  ExampleClient client("localhost", 4444);
  while (true) {
    usleep(100000);
  }
}
