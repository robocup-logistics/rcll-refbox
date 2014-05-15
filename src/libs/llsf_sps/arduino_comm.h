
/***************************************************************************
 *  arduino_comm.h - LLSF RefBox Arduino Machine Communication
 *
 *  Created: Mon Apr 14 19:03:44 2014
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

#ifndef __LLSF_REFBOX_LLSF_SPS_ARDUINO_COMM_H_
#define __LLSF_REFBOX_LLSF_SPS_ARDUINO_COMM_H_

#include <llsf_sps/machine_comm.h>
#include <xbee/xbee.h>

#include <string>
#include <cstdint>
#include <map>

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define SPS_NUM_MACHINES 16

class ArduinoXBeeComm : public MachineCommunication
{
 public:
  ArduinoXBeeComm(const char *usb_device, bool discover = true);
  ~ArduinoXBeeComm();

  void try_reconnect();

  void reset_lights();
  void set_light(unsigned int m, Light light, SignalState state);

  void reset_rfids();
  bool read_rfid(unsigned int m, uint32_t &id);
  std::map<std::string, uint32_t> read_rfids();
  void write_rfid(unsigned int m, uint32_t id);

  bool machine_exists(unsigned int m);

 private:
  void message_received(std::shared_ptr<llsf_xbee::XBeeRxData> rx_data);

 private:
  llsf_xbee::XBee *xbee_;
  std::map<std::string, uint32_t> rfid_tags_;
  std::map<unsigned int, llsf_xbee::XBeeNode> machine_idx_to_node_;
  std::map<uint64_t, llsf_xbee::XBeeNode> hw_addr_to_node_;
};

} // end of namespace llsfrb

#endif
