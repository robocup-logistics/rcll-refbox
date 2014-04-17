
/***************************************************************************
 *  xbee.h - LLSF RefBox XBee Communication
 *
 *  Created: Tue Apr 15 21:48:39 2014
 *  Copyright  2014  Tim Niemueller [www.niemueller.de]
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

#ifndef __LLSF_REFBOX_XBEE_XBEE_H_
#define __LLSF_REFBOX_XBEE_XBEE_H_

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

namespace llsf_xbee {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define XBEE_API_ID_MODEM_STATUS                         0x8A
#define XBEE_API_ID_AT_COMMAND                           0x08
#define XBEE_API_ID_AT_COMMAND_QUEUE_VALUE               0x09
#define XBEE_API_ID_AT_COMMAND_RESPONSE                  0x88
#define XBEE_API_ID_REMOTE_COMMAND_REQUEST               0x17
#define XBEE_API_ID_REMOTE_COMMAND_RESPONSE              0x97
#define XBEE_API_ID_ZIGBEE_TRANSMIT_REQUEST              0x10
#define XBEE_API_ID_EXPLICIT_ADDR_ZIGBEE_COMMAND_FRAME   0x11
#define XBEE_API_ID_ZIGBEE_TRANSMIT_STATUS               0x8B
#define XBEE_API_ID_ZIGBEE_RECEIVE_PACKET                0x90
#define XBEE_API_ID_ZIGBEE_EXPLICIT_RX_INDICATOR         0x91
#define XBEE_API_ID_ZIGBEE_IO_DATA_SAMPLE_RX_INDICATOR   0x92
#define XBEE_API_ID_XBEE_SENSOR_READ_INDICATOR           0x94
#define XBEE_API_ID_NODE_ID_INDICATOR                    0x95

#define XBEE_API_AT_RESP_STATUS_OK                0x00
#define XBEE_API_AT_RESP_STATUS_ERROR             0x01
#define XBEE_API_AT_RESP_STATUS_INVALID_COMMAND   0x02
#define XBEE_API_AT_RESP_STATUS_INVALID_PARAMETER 0x03

#define XBEE_MAX_TX_PAYLOAD_SIZE 72

#pragma pack(push,1)
typedef struct {
  uint8_t  start_delimiter;
  uint16_t length;
  uint8_t  api_id;
} XBeePacketHeader;
#pragma pack(pop)

class XBeeNode {
 public:
  typedef enum {
    COORDINATOR = 0,
    ROUTER = 1,
    END_DEVICE = 2
  } DeviceType;

  XBeeNode();
  XBeeNode(const uint8_t *nd_response, uint16_t response_length);

  uint16_t       network_address;
  uint64_t       hw_address;
  std::string    identifier;
  uint16_t       parent_network_address;
  DeviceType     device_type;
  uint16_t       profile_id;
  uint16_t       manufacturer_id;
};

class XBeePacket {
 public:
  XBeePacket(uint8_t api_id, uint8_t *packet_data, uint16_t payload_length);
  XBeePacket(XBeePacketHeader &header, uint8_t *payload);
  ~XBeePacket();

  uint8_t  api_id() const;
  uint16_t length() const;
  uint16_t payload_length() const;
  bool     has_frame_id() const;
  uint8_t  frame_id() const;

  const uint8_t *  payload() const;

  void print() const;

 public:
  uint8_t  *data;
  uint16_t  data_length;
};

class XBee {
 public:
  XBee(const std::string &device, unsigned int baud_rate = 9600);
  ~XBee();

  void     discover();
  bool     find_node(std::string node_name, uint64_t &hw_addr, uint16_t &net_addr);
  uint16_t get_pan_id();
  uint16_t get_network_address();
  uint64_t get_hardware_address();
  std::map<uint64_t, XBeeNode> get_nodes();

  std::shared_ptr<XBeePacket>
    at_command(const char *command, uint8_t *value, uint8_t value_size, bool wait = true);
  std::shared_ptr<XBeePacket>
    at_command(const char *command, bool wait = true);

  void send(uint64_t hw_addr, uint16_t net_addr, uint8_t *payload, uint8_t payload_size);

  void set_debug(bool debug);

 private:
  void start_recv(bool have_start = false);
  void start_recv_payload();
  void handle_recv_header(const boost::system::error_code& error);
  void handle_recv_payload(const boost::system::error_code& error);
  void handle_write(const boost::system::error_code& error,
		    size_t bytes_transferred, XBeePacket *packet);
  uint8_t next_frame_id()
  {
    // zero means ignore frame id
    if (++frame_id_ == 0)  frame_id_ = 1;
    return frame_id_;
  }
  void send_packet(uint8_t api_id, uint8_t *payload, uint16_t payload_size);
  void wait_response(uint8_t frame_id, std::unique_lock<std::mutex> &lock);
  uint16_t get_uint16(const char *command);
  uint32_t get_uint32(const char *command);

 private:
  bool debug_;
  boost::asio::io_service       io_service_;
  boost::asio::serial_port      serial_;
  boost::asio::io_service::work io_service_work_;

  std::thread asio_thread_;

  XBeePacketHeader in_header_;
  uint8_t *in_payload_;

  std::queue<XBeePacket *> outbound_queue_;
  std::mutex               outbound_mutex_;
  bool                     outbound_active_;

  std::mutex                  inbound_mutex_;
  std::condition_variable     inbound_waitcond_;
  std::shared_ptr<XBeePacket> inbound_packet_;

  uint8_t  frame_id_;

  std::map<uint64_t, XBeeNode> nodes_;
};

} // end of namespace llsfrb

#endif
