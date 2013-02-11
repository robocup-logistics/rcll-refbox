
/***************************************************************************
 *  sps_comm.h - LLSF RefBox SPS Communication
 *
 *  Created: Tue Jan 22 11:00:53 2013
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

#ifndef __LLSF_REFBOX_SPS_COMM_H_
#define __LLSF_REFBOX_SPS_COMM_H_

extern "C" {
  typedef struct _modbus modbus_t;
}

#include <string>
#include <stdint.h>
#include <vector>
#include <map>

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define SPS_NUM_MACHINES 16

class SPSComm
{
 public:
  /** Light signal state. */
  typedef enum {
    SIGNAL_OFF = 0,	//< Light is turned off
    SIGNAL_ON,		//< Light is turned on
    SIGNAL_BLINK	//< Light is blinking.
  } SignalState;

  /** Machine type. */
  typedef enum {
    M_BEGIN = 0, M1 = 0, M2 = 1, M3 = 2, M4 = 3, M5 = 4, M6 = 5, M7 = 6, M8 = 7,
    M9 = 8, M10 = 9, D1 = 10, D2 = 11, D3 = 12, TEST = 13, R1 = 14, R2 = 15, M_END = 16
  } Machine;

  /** Light type. */
  typedef enum {
    LIGHT_BEGIN = 0,	//< Iterator start.
    LIGHT_RED = 0,	//< Red light.
    LIGHT_YELLOW = 1,	//< Yellow light.
    LIGHT_GREEN = 2,	//< Green light.
    LIGHT_END		// Iterator end.
  } Light;

  enum {
    NO_PUCK = 0xFFFFFFFF
  };

  SPSComm(const char *host, unsigned short port);
  ~SPSComm();

  void reset_lights();
  void test_lights();
  void set_light(Machine m, Light light, SignalState state);
  void set_light(std::string &m, std::string  &light, std::string &state);

  void reset_rfids();
  bool read_rfid(Machine m, uint32_t &id);
  std::vector<uint32_t> read_rfids();
  void write_rfid(Machine m, uint32_t id);

  std::string &  index_to_name(uint32_t index);
  Machine      to_machine(std::string &machine);
  Light        to_light(std::string &light);
  SignalState  to_signal_state(std::string &signal_state);

 private:
  modbus_t *mb_;
  std::vector<std::string> index_to_name_;
  std::map<std::string, Machine>     name_to_machine_;
  std::map<std::string, Light>       name_to_light_;
  std::map<std::string, SignalState> name_to_signal_state_;
};

} // end of namespace llsfrb

#endif
