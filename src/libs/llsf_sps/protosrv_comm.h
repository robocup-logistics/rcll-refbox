
/***************************************************************************
 *  protosrv_comm.h - LLSF RefBox protobuf_comm server machine communication
 *
 *  Created: Thu Jul 03 11:03:50 2014
 *  Copyright  2013-2014  Tim Niemueller [www.niemueller.de]
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

#ifndef __LLSF_REFBOX_LLSF_SPS_PROTOSRV_COMM_H_
#define __LLSF_REFBOX_LLSF_SPS_PROTOSRV_COMM_H_

#include <llsf_sps/machine_comm.h>
#include <protobuf_comm/server.h>
#include <msgs/MachineInfo.pb.h>

#include <string>
#include <cstdint>
#include <map>

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define SPS_NUM_MACHINES 16

class ProtobufServerComm : public MachineCommunication
{
 public:
  ProtobufServerComm(unsigned short tcp_port, unsigned int wait_ms = 0);
  ~ProtobufServerComm();

  void try_reconnect();

  void reset_lights();
  void set_light(unsigned int m, Light light, SignalState state);

  void reset_rfids();
  bool read_rfid(unsigned int m, uint32_t &id);
  std::map<std::string, uint32_t> read_rfids();
  void write_rfid(unsigned int m, uint32_t id);

  bool machine_exists(unsigned int m);

 private:
  typedef struct {
    bool                                           connected;
    protobuf_comm::ProtobufStreamServer::ClientID  client_id;
    llsf_msgs::MachineInfo                         machine_info;
    uint32_t                                       puck_id;
  } MachineDetails;

  void machine_ping(std::string &mname,
		    protobuf_comm::ProtobufStreamServer::ClientID client_id);

  void handle_client_connected(protobuf_comm::ProtobufStreamServer::ClientID client,
			       boost::asio::ip::tcp::endpoint &endpoint);
  void handle_client_disconnected(protobuf_comm::ProtobufStreamServer::ClientID client,
				  const boost::system::error_code &error);

  void handle_client_msg(protobuf_comm::ProtobufStreamServer::ClientID client,
			 uint16_t component_id, uint16_t msg_type,
			 std::shared_ptr<google::protobuf::Message> msg);

  void handle_client_fail(protobuf_comm::ProtobufStreamServer::ClientID client,
			  uint16_t component_id, uint16_t msg_type,
			  std::string msg);

  std::string machine_name(protobuf_comm::ProtobufStreamServer::ClientID client_id);

 private:
  protobuf_comm::ProtobufStreamServer  *server_;
  std::map<std::string, MachineDetails> machines_;
};

} // end of namespace llsfrb

#endif
