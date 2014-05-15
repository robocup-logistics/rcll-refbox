
/***************************************************************************
 *  machine_comm.cpp - LLSF RefBox Machine Communication
 *
 *  Created: Sun Apr 20 12:09:37 2014
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

#include <llsf_sps/machine_comm.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>

#include <core/exception.h>
#include <core/exceptions/software.h>

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define LLSF_MACH_FIELD_SET_NUM 16


/** @class MachineCommnuication "machine_comm.h"
 * Abstract machine communication interface.
 * This class provides the generic interface to the LLSF field machines.
 * Specific implementations can then provide access for specific hardware
 * types.
 * @author Tim Niemueller
 */


/** Constructor.
 * @param num_machines number of machines currently in the game
 */
MachineCommunication::MachineCommunication(unsigned int num_machines)
  : num_machines_(num_machines)
{
  construct_mappings();
}

void
MachineCommunication::construct_mappings()
{
  name_to_light_["RED"]    = LIGHT_RED;
  name_to_light_["YELLOW"] = LIGHT_YELLOW;
  name_to_light_["GREEN"]  = LIGHT_GREEN;

  name_to_signal_state_["OFF"]    = SIGNAL_OFF;
  name_to_signal_state_["ON"]     = SIGNAL_ON;
  name_to_signal_state_["BLINK"]  = SIGNAL_BLINK;
}

/** Destructor. */
MachineCommunication::~MachineCommunication()
{
}

/** @fn void MachineCommunication::try_reconnect()
 * Try to reconnect to machines.
 * Called if an error occured while trying to read or write data
 * from or to a machine.
 */


/** Light testing sequence
 * Goes through the following sequence:
 * <ul>
 *  <li>Blink all lights of all machines for 2 seconds</li>
 *  <li>Set all lights of all machines for 5 seconds</li>
 *  <li>Turn on each color of all machines fo 250ms each</li>
 *  <li>Turn on all lights of all machines for two seconds</li>
 *  <li>Turn of all lights of one machine after the other,
 *      each 150ms one machine is turned off.</li>
 * </ul>
 * The operation is blocking until the full sequence has completed.
 */
void
MachineCommunication::test_lights()
{
  // yes, we could use modbus_write_registers to write it all at once,
  // but that would not test the typical code path

  // Set all to blink
  for (unsigned int m = 0; m < num_machines_; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light(m, (Light)l, SIGNAL_BLINK);
    }
  }
  usleep(2000000);

  // Set all ON
  for (unsigned int m = 0; m < num_machines_; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light(m, (Light)l, SIGNAL_ON);
    }
  }
  usleep(500000);

  // Turn on one color at a time
  for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
    reset_lights();
    for (unsigned int m = 0; m < num_machines_; ++m) {
      set_light(m, (Light)l, SIGNAL_ON);
    }
    usleep(200000);
  }

  // Turn all OFF
  for (unsigned int m = 0; m < num_machines_; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light(m, (Light)l, SIGNAL_OFF);
    }
  }
  usleep(1000000);

  // Turn them on one after another
  for (unsigned int m = 0; m < LLSF_MACH_FIELD_SET_NUM; ++m) {
    for (unsigned int s = 0; s < num_machines_ / LLSF_MACH_FIELD_SET_NUM; ++s) {
      for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
        set_light(s * LLSF_MACH_FIELD_SET_NUM + m, (Light)l, SIGNAL_ON);
      }
    }
    usleep(50000);
  }
}


/** @fn void MachineCommunication::reset_lights()
 * Reset all light register to zero (turn off all lights).
 */

/** @fn void MachineCommunication::set_light(unsigned int m, Light light, SignalState state)
 * Set a light of a machine to given state.
 * @param m machine of which to set the light
 * @param light light color to set
 * @param state desired signal state of the light
 */

/** Set a light of a machine to given state.
 * @param m machine of which to set the light
 * @param light light color to set
 * @param state desired signal state of the light
 */
void
MachineCommunication::set_light(unsigned int m, std::string &light, std::string &state)
{
  set_light(m, to_light(light), to_signal_state(state));
}


/** @fn bool MachineCommunication::read_rfid(unsigned int m, uint32_t &id)
 * Read puck ID via RFID.
 * @param m machine of which to read the puck
 * @param id upon returning true the read ID, unmodified otherwise
 * @return true if a puck was successfully read, false if there
 * is no puck under the reader or the communication was interrupted.
 */

/** @fn std::map<std::string, uint32_t> MachineCommunication::read_rfids()
 * Read puck IDs via RFID.
 * @return vector of IDs read from all of the RFID machines. If an
 * idea is 0xFFFFFFFF then no puck was placed below the sensor.
 */


/** @fn void MachineCommunication::reset_rfids()
 * Reset all RFID registers.
 * Call this at the very beginning to avoid false values to be
 * written to a puck.
 */


/** @fn void MachineCommunication::write_rfid(unsigned int m, uint32_t id)
 * Write an ID to a puck using RFID.
 * @param m machine where to write
 * @param id ID to set on the puck
 */


/** Convert string to Light enum.
 * @param light name of light
 * @return corresponding Light value
 */
MachineCommunication::Light
MachineCommunication::to_light(std::string &light)
{
  std::map<std::string, Light>::iterator l;
  if ((l = name_to_light_.find(light)) != name_to_light_.end()) {
    return l->second;
  } else {
    throw fawkes::Exception("Unknown light name '%s' requested", light.c_str());
  }
}

/** Convert string to signal state enum.
 * @param signal_state name of signal state
 * @return corresponding SignalState value
 */
MachineCommunication::SignalState
MachineCommunication::to_signal_state(std::string &signal_state)
{
  std::map<std::string, SignalState>::iterator s;
  if ((s = name_to_signal_state_.find(signal_state)) != name_to_signal_state_.end()) {
    return s->second;
  } else {
    throw fawkes::Exception("Unknown signal state '%s' requested", signal_state.c_str());
  }
}


} // end of namespace llsfrb
