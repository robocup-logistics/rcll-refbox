
/***************************************************************************
 *  arduino_comm.cpp - LLSF RefBox ARDUINO Communication
 *
 *  Created: Mon Apr 21 23:44:26 2014
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

#include <llsf_sps/arduino_comm.h>

#include <core/exception.h>
#include <core/exceptions/software.h>
#include <utils/llsf/machines.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>


using namespace llsf_xbee;

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define AXB_NUM_MACHINES 16

#define MSG_TYPE_SET_RED          2
#define MSG_TYPE_SET_YELLOW       3
#define MSG_TYPE_SET_GREEN        4
#define MSG_TYPE_RFID_DATA        5
#define MSG_TYPE_RFID_REMOVED     6


/** @class ArduinoXBeeComm "arduino_comm.h"
 * Communication handler to Arduino-based field machines.
 * @author Tim Niemueller
 */


/** Constructor.
 * @param usb_device USB device file where ZigBee controller is connected
 */
ArduinoXBeeComm::ArduinoXBeeComm(const char *usb_device, bool discover)
  : MachineCommunication(AXB_NUM_MACHINES)
{
  xbee_ = new XBee(usb_device);
  xbee_->signal_rcvd_rx_data()
    .connect(boost::bind(&ArduinoXBeeComm::message_received, this, _1));

  for (unsigned int i = 0; i < AXB_NUM_MACHINES; ++i) {
    rfid_tags_[to_string(i, llsf_utils::ASSIGNMENT_2014)] = NO_PUCK;
  }

  if (discover) {
    try_reconnect();
  }
}


/** Destructor. */
ArduinoXBeeComm::~ArduinoXBeeComm()
{
  delete xbee_;
}

void
ArduinoXBeeComm::try_reconnect()
{
  xbee_->discover();

  std::map<uint64_t, XBeeNode> nodes = xbee_->get_nodes();
  for (auto n : nodes) {
    unsigned int m = llsf_utils::to_machine(n.second.identifier,
					    llsf_utils::ASSIGNMENT_2014);
    machine_idx_to_node_[m] = n.second;
  }
  hw_addr_to_node_ = nodes;
}


/** Reset all light register to zero (turn off all lights). */
void
ArduinoXBeeComm::reset_lights()
{
}

/** Set a light of a machine to given state.
 * @param m machine of which to set the light
 * @param light light color to set
 * @param state desired signal state of the light
 */
void
ArduinoXBeeComm::set_light(unsigned int m, Light light, SignalState state)
{
  if (! machine_exists(m)) {
    throw fawkes::Exception("Machine %u unknown", m);
  }

  if (machine_idx_to_node_.find(m) != machine_idx_to_node_.end()) {
    uint8_t msg[2];
    msg[1] = state;

    switch (light) {
    case LIGHT_YELLOW: msg[0] = MSG_TYPE_SET_YELLOW; break;
    case LIGHT_GREEN:  msg[0] = MSG_TYPE_SET_GREEN;  break;
    default:           msg[0] = MSG_TYPE_SET_RED;    break;
    }

    XBeeNode &node = machine_idx_to_node_[m];
    xbee_->send(node.hw_address, node.network_address, msg, 2);
  } else {
    throw fawkes::Exception("Node %u unknown", m);
  }
}


/** Read puck ID via RFID.
 * @param m machine of which to read the puck
 * @param id upon returning true the read ID, unmodified otherwise
 * @return true if a puck was successfully read, false if there
 * is no puck under the reader or the communication was interrupted.
 */
bool
ArduinoXBeeComm::read_rfid(unsigned int m, uint32_t &id)
{
  if (! machine_exists(m)) {
    throw fawkes::Exception("Machine %u unknown", m);
  }

  std::string m_name = to_string(m, llsf_utils::ASSIGNMENT_2014);
  if (rfid_tags_.find(m_name) != rfid_tags_.end() && rfid_tags_[m_name] != NO_PUCK) {
    id = rfid_tags_[m_name];
    return true;
  } else {
    return false;
  }
}

/** Read puck IDs via RFID.
 * @return vector of IDs read from all of the RFID machines. If an
 * idea is 0xFFFFFFFF then no puck was placed below the sensor.
 */
std::map<std::string, uint32_t>
ArduinoXBeeComm::read_rfids()
{
  return rfid_tags_;
}


/** Reset all RFID registers.
 * Call this at the very beginning to avoid false values to be
 * written to a puck.
 */
void
ArduinoXBeeComm::reset_rfids()
{
  for (unsigned int i = 0; i < AXB_NUM_MACHINES; ++i) {
    rfid_tags_[to_string(i, llsf_utils::ASSIGNMENT_2014)] = NO_PUCK;
  }  
}


/** Write an ID to a puck using RFID.
 * @param m machine where to write
 * @param id ID to set on the puck
 */
void
ArduinoXBeeComm::write_rfid(unsigned int m, uint32_t id)
{
  throw fawkes::Exception("Not supported atm");
}


bool
ArduinoXBeeComm::machine_exists(unsigned int m)
{
  if (machine_idx_to_node_.find(m) == machine_idx_to_node_.end()) {
    // try to find this specific machine
    std::string machine_name = to_string(m, llsf_utils::ASSIGNMENT_2014);
    XBeeNode node;
    if (xbee_->find_node(machine_name, node)) {
      machine_idx_to_node_[m] = node;
      hw_addr_to_node_[node.hw_address] = node;
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}

void
ArduinoXBeeComm::message_received(std::shared_ptr<XBeeRxData> rx_data)
{
  const uint8_t *payload = rx_data->payload();
  if (rx_data->length() >= 1) {
    switch (payload[0]) {
    case MSG_TYPE_RFID_DATA:
      if (hw_addr_to_node_.find(rx_data->sender_hw_addr()) != hw_addr_to_node_.end()) {
	std::string &node_name = hw_addr_to_node_[rx_data->sender_hw_addr()].identifier;
	if (rx_data->length() == 5) {
	  uint32_t *tag = (uint32_t *)&payload[1];
	  //printf("Tag: %x\n", *tag);
	  rfid_tags_[node_name] = *tag;
	} else {
	  printf("Invalid rfid data packet received\n");
	}
      }
      break;
    case MSG_TYPE_RFID_REMOVED:
      if (hw_addr_to_node_.find(rx_data->sender_hw_addr()) != hw_addr_to_node_.end()) {
	std::string &node_name = hw_addr_to_node_[rx_data->sender_hw_addr()].identifier;
	rfid_tags_[node_name] = NO_PUCK;
      }
      break;
    }
  }
}


} // end of namespace llsfrb
