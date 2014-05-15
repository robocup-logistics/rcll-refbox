
/***************************************************************************
 *  machine_comm.h - LLSF RefBox Machine Communication
 *
 *  Created: Sat Apr 19 17:22:48 2014
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

#ifndef __LLSF_REFBOX_MACHINE_COMM_H_
#define __LLSF_REFBOX_MACHINE_COMM_H_

#include <string>
#include <stdint.h>
#include <vector>
#include <map>

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class MachineCommunication
{
 public:
  /** Light signal state. */
  typedef enum {
    SIGNAL_OFF = 0,	//< Light is turned off
    SIGNAL_ON,		//< Light is turned on
    SIGNAL_BLINK	//< Light is blinking.
  } SignalState;

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

  virtual ~MachineCommunication();

  virtual void try_reconnect() = 0;

  virtual void reset_lights() = 0;
  void test_lights();

  void set_light(unsigned int m, std::string &light, std::string &state);
  virtual void set_light(unsigned int m, Light light, SignalState state) = 0;

  virtual void reset_rfids() = 0;
  virtual bool read_rfid(unsigned int m, uint32_t &id) = 0;
  virtual std::map<std::string, uint32_t> read_rfids() = 0;
  virtual void write_rfid(unsigned int m, uint32_t id) = 0;

  Light        to_light(std::string &light);
  SignalState  to_signal_state(std::string &signal_state);

 protected:
  MachineCommunication(unsigned int num_machines);

 private:
  void construct_mappings();

 protected:
  const unsigned int                 num_machines_;

 private:
  std::map<std::string, Light>       name_to_light_;
  std::map<std::string, SignalState> name_to_signal_state_;
};

} // end of namespace llsfrb

#endif
