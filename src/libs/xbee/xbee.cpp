
/***************************************************************************
 *  xbee.cpp - LLSF RefBox XBee Communication
 *
 *  Created: Tue Apr 15 21:54:30 2014
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

#include <xbee/xbee.h>

#define START_BYTE  0x7e
#define ESCAPE_BYTE 0x7d
#define XON_BYTE    0x11
#define XOFF_BYTE   0x13

#define XBEE_PACKET_API_ID_INDEX        3
#define XBEE_PACKET_PAYLOAD_START_INDEX 4
#define XBEE_PACKET_FRAME_ID_INDEX      XBEE_PACKET_PAYLOAD_START_INDEX

#define XBEE_PACKET_AT_RESPONSE_COMMAND_INDEX 1


namespace llsf_xbee {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class XBee <xbee/xbee.h>
 * Communicate via XBee Series 2 (ZigBee) adapters.
 * Simple class that allows asynchronous sending and receiving of XBee
 * messages. This has been tested with Sparkfun XBee Explorer USB.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param device device file of serial adapter where XBee is connected
 * @param baud_rate baud rate to use to talk to the device
 */
XBee::XBee(const std::string &device, unsigned int baud_rate)
  : serial_(io_service_, device), io_service_work_(io_service_)
{
  debug_ = false;
  frame_id_ = 0;
  in_payload_ = 0;
  outbound_active_ = false;
  serial_.set_option(boost::asio::serial_port::baud_rate(baud_rate));
  asio_thread_ = std::thread([this]() { this->io_service_.run(); });
  start_recv();
}

/** Destructor. */
XBee::~XBee()
{
  io_service_.stop();
  asio_thread_.join();
  delete in_payload_;
}


/** Enable or disable debugging.
 * @param debug true to enable, false to disable debugging output
 */
void
XBee::set_debug(bool debug)
{
  debug_ = debug;
}


/** Start receiving data.
 * Called internally.
 * @param have_start true if the start byte has already been received, e.g.
 * on recovering from a communication failure
 */
void
XBee::start_recv(bool have_start)
{
  if (have_start) {
    // we already read the start byte
    char *buf = (char *)&in_header_;
    in_header_.start_delimiter = START_BYTE;
    boost::asio::async_read(serial_,
			    boost::asio::buffer(buf+1, sizeof(XBeePacketHeader)-1),
			    boost::bind(&XBee::handle_recv_header,
					this, boost::asio::placeholders::error));
  } else {
    boost::asio::async_read(serial_,
			    boost::asio::buffer(&in_header_, sizeof(XBeePacketHeader)),
			    boost::bind(&XBee::handle_recv_header,
					this, boost::asio::placeholders::error));
  }
}


/** Start to receive payload.
 * Called internally. */
void
XBee::start_recv_payload()
{
  delete in_payload_;

  // +1: checksum, -1: API ID (already read as header)
  in_payload_ = new uint8_t[in_header_.length];

  boost::asio::async_read(serial_,
			  boost::asio::buffer(in_payload_, in_header_.length),
			  boost::bind(&XBee::handle_recv_payload,
				      this, boost::asio::placeholders::error));
}


/** Handle reception of header.
 * @param error error status
 */
void
XBee::handle_recv_header(const boost::system::error_code& error)
{
  if (! error) {
    if (in_header_.start_delimiter != START_BYTE) {
      // error
      printf("FAIL: invalid start byte\n");
      boost::asio::streambuf b;
      boost::asio::read_until(serial_, b, START_BYTE);
      start_recv(true);
    } else {
      in_header_.length = ntohs(in_header_.length);

      start_recv_payload();
    }
  } else {
    printf("Failed to receive header\n");
    start_recv();
  }
}


/** Handle reception of payload.
 * @param error error status
 */
void
XBee::handle_recv_payload(const boost::system::error_code& error)
{
  if (! error) {
    uint32_t checksum = in_header_.api_id;
    // this loop already includes the checksum because header.length
    // would include the API ID (which we already take as start value
    for (unsigned int i = 0; i < in_header_.length; ++i) {
      checksum += in_payload_[i];
    }

    if ((checksum & 0xff) != 0xff) {
      printf("Checksum fail\n");
    } else {
      std::unique_lock<std::mutex> lock(inbound_mutex_);
      inbound_packet_.reset(new XBeePacket(in_header_, in_payload_));

      if (debug_) {
	printf("Received: ");
	inbound_packet_->print();
      }

      if (inbound_packet_->api_id() == XBEE_API_ID_AT_COMMAND_RESPONSE) {
	// if this is a discover result, add a note for it
	const uint8_t *payload = inbound_packet_->payload();
	if (payload[XBEE_PACKET_AT_RESPONSE_COMMAND_INDEX    ] == 'N' &&
	    payload[XBEE_PACKET_AT_RESPONSE_COMMAND_INDEX + 1] == 'D')
	{
	  XBeeNode node(payload, inbound_packet_->payload_length());
	  nodes_[node.hw_address] = node;
	}
      }
      inbound_waitcond_.notify_all();
      lock.unlock();
      std::this_thread::yield();
    }
  } else {
    printf("Failed to receive payload\n");
  }
  start_recv();
}


/** Send an AT command and optionally wait for response.
 * @param command exactly two characters denoting the AT command
 * @param value the value to be sent, can be NULL
 * @param value_size length in bytes of @p value, maybe zero
 * @param wait true to wait for a response, false not to
 * @return received package, if waiting disabled just returns an empty
 * packet, otherwise the response.
 */
std::shared_ptr<XBeePacket>
XBee::at_command(const char *command, uint8_t *value, uint8_t value_size, bool wait)
{
  if (strlen(command) != 2) {
    throw std::runtime_error("Command must be two bytes");
  }
  // frame ID, at command, value
  uint16_t payload_size = 1 + 2 + value_size;
  uint8_t  payload[payload_size];
  payload[0] = next_frame_id();
  payload[1] = command[0];
  payload[2] = command[1];
  for (unsigned int i = 0; i < value_size; ++i) {
    payload[i+3] = value[i];
  }

  std::unique_lock<std::mutex> lock(inbound_mutex_);
  inbound_packet_.reset();

  send_packet(XBEE_API_ID_AT_COMMAND, payload, payload_size);

  if (wait) {
    wait_response(payload[0], lock);

    return inbound_packet_;
  } else {
    return std::shared_ptr<XBeePacket>();
  }
}

/** Send an AT command without a value and optionally wait for response.
 * @param command exactly two characters denoting the AT command
 * @param wait true to wait for a response, false not to
 * @return received package, if waiting disabled just returns an empty
 * packet, otherwise the response.
 */
std::shared_ptr<XBeePacket>
XBee::at_command(const char *command, bool wait)
{
  return at_command(command, NULL, 0, wait);
}


/** Send data to another node.
 * @param hw_addr hardware address of node to send to
 * @param net_addr network address of node to send to
 * @param payload payload to send
 * @param payload_size size of @p payload in bytes
 */
void
XBee::send(uint64_t hw_addr, uint16_t net_addr, uint8_t *payload, uint8_t payload_size)
{
  if (payload_size > XBEE_MAX_TX_PAYLOAD_SIZE) {
    throw std::runtime_error("Payload too large to send");
  }

  uint16_t buffer_size = 13 + payload_size;
  uint8_t buffer[buffer_size];
  buffer[0] = next_frame_id();
  // hw address, set to zero, using network address
  //for (unsigned int i = 1; i <= 8; ++i)  buffer[i] = 0;
  buffer[1]  = (hw_addr >> 56) & 0xff;
  buffer[2]  = (hw_addr >> 48) & 0xff;
  buffer[3]  = (hw_addr >> 40) & 0xff;
  buffer[4]  = (hw_addr >> 32) & 0xff;
  buffer[5]  = (hw_addr >> 24) & 0xff;
  buffer[6]  = (hw_addr >> 16) & 0xff;
  buffer[7]  = (hw_addr >> 8) & 0xff;
  buffer[8]  = hw_addr & 0xff;
  buffer[9]  = (net_addr >> 8) & 0xff;
  buffer[10] = net_addr & 0xff;
  buffer[11] = 0; // broadcast radius, use default
  buffer[12] = 0; // options, use unicast
  for (unsigned int i = 0; i < payload_size; ++i) {
    buffer[13 + i] = payload[i];
  }
  send_packet(XBEE_API_ID_ZIGBEE_TRANSMIT_REQUEST, buffer, buffer_size);
}

void
XBee::wait_response(uint8_t frame_id, std::unique_lock<std::mutex> &lock)
{
  while (inbound_packet_ == NULL || inbound_packet_->frame_id() != frame_id) {
    inbound_waitcond_.wait(lock);
  }
}

void
XBee::send_packet(uint8_t api_id, uint8_t *payload, uint16_t payload_size)
{
  XBeePacket *entry = new XBeePacket(api_id, payload, payload_size);
 
  std::lock_guard<std::mutex> lock(outbound_mutex_);
  if (outbound_active_) {
    outbound_queue_.push(entry);
  } else {
    outbound_active_ = true;
    if (debug_) {
      printf("Sending: ");
      for (unsigned int i = 0; i < entry->data_length; ++i) {
	printf("%02x", entry->data[i]);
      }
      printf("\n");
    }
    boost::asio::async_write(serial_,
			     boost::asio::buffer(entry->data, entry->data_length),
			     boost::bind(&XBee::handle_write, this,
					 boost::asio::placeholders::error,
					 boost::asio::placeholders::bytes_transferred,
					 entry));
  }

  /*
  uint8_t *data = (uint8_t *)payload;
  uint16_t escaped_bytes = 0;
  for (uint16_t i = 0; i < payload_size; ++i) {
    if (data[i] == ESCAPE_BYTE || data[i] == START_BYTE ||
	data[i] == XON_BYTE || data[i] == XOFF_BYTE)
    {
      escaped_bytes += 1;
    }
  }
  if ((payload_size >> 8) == ESCAPE_BYTE) {
    escaped_bytes += 1;
  }
  if ((payload_size & 0xff) == ESCAPE_BYTE) {
    escaped_bytes += 1;
  }

  // 5: start byte, length/2, API ID, checksum
  uint16_t buffer_size = 5 + payload_size + escaped_bytes;
  uint8_t *out_data = new uint8_t[buffer_size];
  uint8_t j = 0;
  out_data[j++] = START_BYTE;
  if ((payload_size >> 8)  == ESCAPE_BYTE) {
    out_data[j++] = ESCAPE_BYTE;
    out_data[j++] = 0x20 ^ ((payload_size >> 8) & 0xff);
  } else {
    out_data[j++] = (payload_size >> 8) & 0xff;
  }
  if ((payload_size & 0xff) == ESCAPE_BYTE) {
    out_data[j++] = ESCAPE_BYTE;
    out_data[j++] = 0x20 ^ (payload_size & 0xff);
  } else {
    out_data[j++] = payload_size & 0xff;
  }
  out_data[j++] = api_id;

  for (uint16_t i = 0; i < payload_size; ++i) {
    if (data[i] == ESCAPE_BYTE || data[i] == START_BYTE ||
	data[i] == XON_BYTE || data[i] == XOFF_BYTE)
    {
      out_data[j++] = ESCAPE_BYTE;
      out_data[j++] = 0x20 ^ data[i];
    } else {
      out_data[j++] = data[i];
    }
  }

  boost::asio::write(serial_, boost::asio::buffer(out_data, buffer_size));

  delete out_data;
*/
}

void
XBee::discover()
{
  at_command("ND", /* wait */ false);
}


bool
XBee::find_node(std::string node_name, uint64_t &hw_addr, uint16_t &net_addr)
{
  std::shared_ptr<XBeePacket> response =
    at_command("DN", (uint8_t*)node_name.c_str(), node_name.length());

  const uint8_t *payload = response->payload();

  // 0: frame id
  // 1,2: AT command
  // 3: status
  // 4,5: net addr
  // 6-13: hw addr

  if (payload[3] == XBEE_API_AT_RESP_STATUS_OK) {
    net_addr = (payload[4] << 8) + payload[5];

    hw_addr =
      ((uint64_t)payload[6]  << 56) + ((uint64_t)payload[7] << 48) +
      ((uint64_t)payload[8]  << 40) + ((uint64_t)payload[9] << 32) +
      ((uint64_t)payload[10] << 24) + ((uint64_t)payload[11] << 16) +
      ((uint64_t)payload[12] << 8)  +  payload[13];

    return true;
  } else {
    return false;
  }
}

uint16_t
XBee::get_uint16(const char *command)
{
  std::shared_ptr<XBeePacket> response = at_command(command);
  // Go requested message
  // 0: frame id
  // 1,2: AT command
  // 3: status
  // N-2,N-1: value
  const uint8_t *payload = response->payload();
  uint16_t p_length = response->payload_length();

  return (payload[p_length - 2] << 8) + payload[p_length-1];
}

uint32_t
XBee::get_uint32(const char *command)
{
  std::shared_ptr<XBeePacket> response = at_command(command);
  // Go requested message
  // 0: frame id
  // 1,2: AT command
  // 3: status
  // N-4,N-3,N-2,N-1: value
  const uint8_t *payload = response->payload();
  uint16_t p_length = response->payload_length();

  return
    (payload[p_length-4] << 24) + (payload[p_length-3] << 16) +
    (payload[p_length-2] << 8) + payload[p_length-1];
}

uint16_t
XBee::get_pan_id()
{
  return get_uint32("ID");
}


uint16_t
XBee::get_network_address()
{
  return get_uint16("MY");
}

uint64_t
XBee::get_hardware_address()
{
  return ((uint64_t)get_uint32("DH") << 32) + get_uint32("DL");
}


std::map<uint64_t, XBeeNode>
XBee::get_nodes()
{
  return nodes_;
}


void
XBee::handle_write(const boost::system::error_code& error,
		   size_t /*bytes_transferred*/, XBeePacket *entry)
{
  delete entry;

  if (! error) {
    std::lock_guard<std::mutex> lock(outbound_mutex_);
    if (! outbound_queue_.empty()) {
      XBeePacket *entry = outbound_queue_.front();
      outbound_queue_.pop();

      if (debug_) {
	printf("Sending: ");
	for (unsigned int i = 0; i < entry->data_length; ++i) {
	  printf("%02x", entry->data[i]);
	}
	printf("\n");
      }

      boost::asio::async_write(serial_,
			       boost::asio::buffer(entry->data, entry->data_length),
			       boost::bind(&XBee::handle_write, this,
					   boost::asio::placeholders::error,
					   boost::asio::placeholders::bytes_transferred,
					   entry));
    } else {
      outbound_active_ = false;
    }
  } else {
    printf("Sending failed\n");
  }
}


XBeePacket::XBeePacket(uint8_t api_id, uint8_t *payload, uint16_t payload_length)
{
  // +1: checksum
  data_length = sizeof(XBeePacketHeader) + payload_length + 1;

  // +1: API ID
  uint16_t payload_length_with_api_id = payload_length + 1;

  data = new uint8_t[data_length];
  data[0] = START_BYTE;
  data[1] = (payload_length_with_api_id >> 8) & 0xff;
  data[2] = payload_length_with_api_id & 0xff;
  data[3] = api_id;

  uint32_t checksum = api_id;
  for (unsigned int i = 0; i < payload_length; ++i) {
    data[i + 4] = payload[i];
    checksum += payload[i];
  }
  data[data_length - 1] = 0xff - (checksum & 0xff);
}


XBeePacket::XBeePacket(XBeePacketHeader &header, uint8_t *payload)
{
  // +1: checksum, -1: API_ID is contained in header.length and in header itself
  data_length = sizeof(XBeePacketHeader) + header.length;

  data = new uint8_t[data_length];
  data[0] = START_BYTE;
  data[1] = (header.length >> 8) & 0xff;
  data[2] = header.length & 0xff;
  data[3] = header.api_id;

  // this is shifted by one and does not include API ID but does include checksum
  memcpy(&data[4], payload, header.length);
}

XBeePacket::~XBeePacket()
{
  delete data;
}

uint8_t
XBeePacket::api_id() const
{
  if ((data_length + 1) < XBEE_PACKET_API_ID_INDEX) {
    return 0;
  }
  return data[XBEE_PACKET_API_ID_INDEX];
}

uint16_t
XBeePacket::length() const
{
  return data_length;
}

uint16_t
XBeePacket::payload_length() const
{
  // -1: checksum
  return data_length - sizeof(XBeePacketHeader) - 1;
}


bool
XBeePacket::has_frame_id() const
{
  if ((data_length + 1) < XBEE_PACKET_FRAME_ID_INDEX) {
    return false;
  }

  switch (data[XBEE_PACKET_API_ID_INDEX]) {
  case XBEE_API_ID_AT_COMMAND:
  case XBEE_API_ID_AT_COMMAND_QUEUE_VALUE:
  case XBEE_API_ID_AT_COMMAND_RESPONSE:
  case XBEE_API_ID_REMOTE_COMMAND_REQUEST:
  case XBEE_API_ID_REMOTE_COMMAND_RESPONSE:
  case XBEE_API_ID_ZIGBEE_TRANSMIT_REQUEST:
  case XBEE_API_ID_EXPLICIT_ADDR_ZIGBEE_COMMAND_FRAME:
  case XBEE_API_ID_ZIGBEE_TRANSMIT_STATUS:
    return true;
  default:
    return false;
  }
}

uint8_t
XBeePacket::frame_id() const
{
  if (has_frame_id()) {
    return data[XBEE_PACKET_FRAME_ID_INDEX];
  } else {
    return 0;
  }
}


const uint8_t *
XBeePacket::payload() const
{
  return &data[XBEE_PACKET_PAYLOAD_START_INDEX];
}

void
XBeePacket::print() const
{
  for (unsigned int i = 0; i <= XBEE_PACKET_API_ID_INDEX; ++i) {
    printf("%02x", data[i]);
  }
  printf(" ");
  for (unsigned int i = XBEE_PACKET_API_ID_INDEX + 1; i < data_length; ++i) {
    printf("%02x", data[i]);
  }
  printf("\n");
}

XBeeNode::XBeeNode()
{
  hw_address = 0;
  network_address = 0;
  parent_network_address = 0;
  profile_id = 0;
  manufacturer_id = 0;
}

XBeeNode::XBeeNode(const uint8_t *nd_response, uint16_t response_length)
{
  //                   0      3   4    6        10       14
  //Received: 7e001b88 024e44 00  66ab 0013a200 408b6838 204d3100fffe0100c105101e00
  // 0: frame ID
  // 1,2: AT command (ND)
  // 3: status
  // 4,5: network address
  // 6-13: hardware address
  // 14-n: network identifier, null-terminated
  // n+1,

  if (nd_response[3] != XBEE_API_AT_RESP_STATUS_OK) {
    throw std::runtime_error("ND response status fail");
  }

  network_address = (nd_response[4] << 8) + nd_response[5];  
  memcpy(&hw_address, &nd_response[6], sizeof(uint64_t));

  hw_address =
    ((uint64_t)nd_response[6]  << 56) + ((uint64_t)nd_response[7] << 48) +
    ((uint64_t)nd_response[8]  << 40) + ((uint64_t)nd_response[9] << 32) +
    ((uint64_t)nd_response[10] << 24) + ((uint64_t)nd_response[11] << 16) +
    ((uint64_t)nd_response[12] << 8)  +  nd_response[13];

  uint32_t ni_length = strnlen((const char *)&nd_response[14], response_length-15);
  identifier = std::string((const char *)&nd_response[14], ni_length);
  parent_network_address =
    (nd_response[14 + ni_length + 2] << 8) + nd_response[14 + ni_length + 3];  
  device_type = (DeviceType)nd_response[14 + ni_length + 4];  
  profile_id =
    (nd_response[14 + ni_length + 6] << 8) + nd_response[14 + ni_length + 7];  
  manufacturer_id =
    (nd_response[14 + ni_length + 8] << 8) + nd_response[14 + ni_length + 9];
}

} // end of namespace llsfrb
