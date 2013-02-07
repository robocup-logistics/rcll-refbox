
/***************************************************************************
 *  refbox.h - LLSF RefBox main program
 *
 *  Created: Thu Feb 07 11:02:51 2013
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

#ifndef __LLSF_REFBOX_REFBOX_H_
#define __LLSF_REFBOX_REFBOX_H_

#include <boost/asio.hpp>
#include <google/protobuf/message.h>
#include <protobuf_comm/server.h>

namespace CLIPS {
  class Environment;
}

namespace protobuf_comm {
  class ProtobufBroadcastPeer;
}

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class SPSComm;
class Configuration;

class LLSFRefBox
{
 public:
  LLSFRefBox(int argc, char **argv);
  ~LLSFRefBox();

  int run();


 private: // methods
  void start_timer();
  void handle_timer(const boost::system::error_code& error);
  void handle_signal(const boost::system::error_code& error, int signum);
  void handle_client_connected(protobuf_comm::ProtobufStreamServer::ClientID client);
  void handle_client_disconnected(protobuf_comm::ProtobufStreamServer::ClientID client,
				  const boost::system::error_code &error);

  void handle_client_msg(protobuf_comm::ProtobufStreamServer::ClientID client,
			 uint16_t component_id, uint16_t msg_type,
			 std::shared_ptr<google::protobuf::Message> msg);

  void handle_peer_msg(boost::asio::ip::udp::endpoint &endpoint,
		       uint16_t component_id, uint16_t msg_type,
		       std::shared_ptr<google::protobuf::Message> msg);

 private: // members
  Configuration *config_;
  SPSComm *sps_;
  protobuf_comm::ProtobufStreamServer *pbc_server_;
  protobuf_comm::ProtobufBroadcastPeer *pbc_peer_;

  CLIPS::Environment *clips_;
  std::mutex          clips_mutex_;

  boost::asio::io_service      io_service_;
  boost::asio::deadline_timer  timer_;
  boost::posix_time::ptime     timer_last_;

  unsigned int cfg_timer_interval_;
};


} // end of namespace llsfrb

#endif
