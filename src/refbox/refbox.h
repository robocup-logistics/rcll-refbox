
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

#include <clipsmm.h>
#include <mutex>

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
  void handle_client_connected(protobuf_comm::ProtobufStreamServer::ClientID client,
			       boost::asio::ip::tcp::endpoint &endpoint);
  void handle_client_disconnected(protobuf_comm::ProtobufStreamServer::ClientID client,
				  const boost::system::error_code &error);

  void handle_client_msg(protobuf_comm::ProtobufStreamServer::ClientID client,
			 uint16_t component_id, uint16_t msg_type,
			 std::shared_ptr<google::protobuf::Message> msg);

  void handle_peer_msg(boost::asio::ip::udp::endpoint &endpoint,
		       uint16_t component_id, uint16_t msg_type,
		       std::shared_ptr<google::protobuf::Message> msg);

  void setup_protobuf_comm();

  void          setup_clips();
  void          handle_clips_periodic();
  void          clips_assert_message(std::pair<std::string, unsigned short> &endpoint,
				     uint16_t comp_id, uint16_t msg_type,
				     std::shared_ptr<google::protobuf::Message> &msg,
				     unsigned int client_id = 0);
  CLIPS::Values clips_now();
  CLIPS::Values clips_get_clips_dirs();
  void          clips_load_config(std::string cfg_prefix);
  CLIPS::Values clips_pb_field_names(void *msgptr);
  CLIPS::Value  clips_pb_field_value(void *msgptr, std::string field_name);
  CLIPS::Value  clips_pb_field_type(void *msgptr, std::string field_name);
  CLIPS::Value  clips_pb_field_label(void *msgptr, std::string field_name);
  CLIPS::Values clips_pb_field_list(void *msgptr, std::string field_name);
  bool          clips_pb_field_is_list(void *msgptr, std::string field_name);
  CLIPS::Value  clips_pb_create(std::string full_name);
  CLIPS::Value  clips_pb_ref(void *msgptr);
  void          clips_pb_destroy(void *msgptr);
  void          clips_pb_set_field(void *msgptr, std::string field_name, CLIPS::Value value);
  void          clips_pb_send(void *msgptr, long int client_id);
  void          clips_pb_disconnect(long int client_id);

  void          clips_sps_set_signal(std::string machine, std::string light, std::string state);
  void          sps_read_rfids();


 private: // members
  Configuration *config_;
  SPSComm *sps_;
  protobuf_comm::ProtobufStreamServer *pbc_server_;
  protobuf_comm::ProtobufBroadcastPeer *pbc_peer_;
  std::map<protobuf_comm::ProtobufStreamServer::ClientID,
    std::pair<std::string, unsigned short>> client_endpoints_;

  CLIPS::Environment                       *clips_;
  std::recursive_mutex                      clips_mutex_;
  std::map<long int, CLIPS::Fact::pointer>  clips_msg_facts_;

  boost::asio::io_service      io_service_;
  boost::asio::deadline_timer  timer_;
  boost::posix_time::ptime     timer_last_;

  unsigned int cfg_timer_interval_;
  std::string  cfg_clips_dir_;
};


} // end of namespace llsfrb

#endif
