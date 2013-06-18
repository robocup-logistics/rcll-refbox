
/***************************************************************************
 *  sps_comm.cpp - LLSF RefBox SPS Communication
 *
 *  Created: Tue Jan 22 11:10:49 2013
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

#include <llsf_sps/sps_comm.h>

#include <modbus.h>
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


#define MB_IN_REG_START   0x4000
#define MB_OUT_REG_START  0x4400

#define SPS_OUT_REG_PER_SIGNAL 3
#define SPS_IN_REG_PER_RFID    3
#define SPS_OUT_REG_PER_RFID   3



#define SPS_OUT_REG_START_SIGNAL MB_OUT_REG_START
#define SPS_OUT_REG_START_RFID						\
  (SPS_OUT_REG_START_SIGNAL + (SPS_OUT_REG_PER_SIGNAL * SPS_NUM_MACHINES))

#define SPS_IN_REG_START_RFID    MB_IN_REG_START

#define SPS_RFID_HAS_PUCK      0x0100
#define SPS_RFID_WRITE_DONE    0x0200
#define SPS_RFID_WRITE_PUCK_ID 0x0100
#define SPS_RFID_RESET_WRITE   0x0200

/** @class SPSComm "sps_comm.h"
 * Communication handler to field SPS.
 * The SPS controls the signal lights and reads and writes from and
 * to the RFID sensors. This class encapsulates the communication via
 * Modbus and provides access to LLSF-specific actions.
 * @author Tim Niemueller
 */


/** Constructor.
 * @param host host of SPS Modbus host
 * @param port TCP port to communicate to, Modbus uses 502 by default
 */
SPSComm::SPSComm(const char *host, unsigned short port)
{
  mb_ = modbus_new_tcp(host, port);
  if (modbus_connect(mb_) == -1) {
    modbus_free(mb_);
    mb_ = NULL;
    throw fawkes::Exception("Failed to connect to SPS: %s",
			    modbus_strerror(errno));
  }

  index_to_name_.resize(SPS_NUM_MACHINES + 1);
  index_to_name_[M1]   = "M1";
  index_to_name_[M2]   = "M2";
  index_to_name_[M3]   = "M3";
  index_to_name_[M4]   = "M4";
  index_to_name_[M5]   = "M5";
  index_to_name_[M6]   = "M6";
  index_to_name_[M7]   = "M7";
  index_to_name_[M8]   = "M8";
  index_to_name_[M9]   = "M9";
  index_to_name_[M10]  = "M10";
  index_to_name_[D1]   = "D1";
  index_to_name_[D2]   = "D2";
  index_to_name_[D3]   = "D3";
  index_to_name_[TST]  = "TST";
  index_to_name_[R1]   = "R1";
  index_to_name_[R2]   = "R2";
  index_to_name_[SPS_NUM_MACHINES]   = "UNKNOWN";

  name_to_machine_["M1"]   = M1;
  name_to_machine_["M2"]   = M2;
  name_to_machine_["M3"]   = M3;
  name_to_machine_["M4"]   = M4;
  name_to_machine_["M5"]   = M5;
  name_to_machine_["M6"]   = M6;
  name_to_machine_["M7"]   = M7;
  name_to_machine_["M8"]   = M8;
  name_to_machine_["M9"]   = M9;
  name_to_machine_["M10"]  = M10;
  name_to_machine_["D1"]   = D1;
  name_to_machine_["D2"]   = D2;
  name_to_machine_["D3"]   = D3;
  name_to_machine_["TST"]  = TST;
  name_to_machine_["R1"]   = R1;
  name_to_machine_["R2"]   = R2;

  name_to_light_["RED"]    = LIGHT_RED;
  name_to_light_["YELLOW"] = LIGHT_YELLOW;
  name_to_light_["GREEN"]  = LIGHT_GREEN;

  name_to_signal_state_["OFF"]    = SIGNAL_OFF;
  name_to_signal_state_["ON"]     = SIGNAL_ON;
  name_to_signal_state_["BLINK"]  = SIGNAL_BLINK;
}


/** Destructor. */
SPSComm::~SPSComm()
{
  if (mb_) {
    modbus_close(mb_);
    modbus_free(mb_);
  }
}

void
SPSComm::try_reconnect()
{
  modbus_close(mb_);
  if (modbus_connect(mb_) == -1) {
    throw fawkes::Exception("Failed to re-connect to SPS: %s",
			    modbus_strerror(errno));
  }
}


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
SPSComm::test_lights()
{
  // yes, we could use modbus_write_registers to write it all at once,
  // but that would not test the typical code path

  // Set all to blink
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_BLINK);
    }
  }
  usleep(2000000);

  // Set all ON
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_ON);
    }
  }
  usleep(500000);

  // Turn on one color at a time
  for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
    reset_lights();
    for (int m = M_BEGIN; m < M_END; ++m) {
      set_light((Machine)m, (Light)l, SIGNAL_ON);
    }
    usleep(200000);
  }

  // Turn all OFF
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_OFF);
    }
  }
  usleep(1000000);

  // Turn them on one after another
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_ON);
    }
    usleep(80000);
  }
}


/** Reset all light register to zero (turn off all lights). */
void
SPSComm::reset_lights()
{
  uint16_t *values = (uint16_t *)calloc(SPS_OUT_REG_PER_SIGNAL * SPS_NUM_MACHINES,
					  sizeof(uint16_t));
  if (modbus_write_registers(mb_, SPS_OUT_REG_START_SIGNAL,
			     SPS_OUT_REG_PER_SIGNAL * SPS_NUM_MACHINES, values) == -1)
  {
    free(values);
    throw fawkes::Exception("Failed to reset register: %s", modbus_strerror(errno));
  }
  free(values);
}

/** Set a light of a machine to given state.
 * @param m machine of which to set the light
 * @param light light color to set
 * @param state desired signal state of the light
 */
void
SPSComm::set_light(Machine m, Light light, SignalState state)
{
  if (light < LIGHT_RED || light > LIGHT_GREEN) {
    throw fawkes::OutOfBoundsException("Signal register offset out of bounds",
				       light, LIGHT_RED, LIGHT_GREEN);
  }

  if (m < M1 || m > R2) {
    throw fawkes::OutOfBoundsException("Machine index out of bounds", m, M1, R2);
  }
  /*
  if (hz < 0 || hz > 255) {
    throw fawkes::OutOfBoundsException("Machine frequency of bounds", hz, 0, 255);    
  }
  */

  uint8_t state_value = 0;
  if (state == SIGNAL_ON) {
    state_value = 1;
  } else if (state == SIGNAL_BLINK) {
    state_value = 2;
  }

  uint16_t value = (state_value << 8); // | (hz & 0x000000ff);

  if (modbus_write_register(mb_,
			    SPS_OUT_REG_START_SIGNAL + m * SPS_OUT_REG_PER_SIGNAL + light,
			    value) == -1)
  {
    throw fawkes::Exception("Failed to set register: %s", modbus_strerror(errno));
  }
}

/** Set a light of a machine to given state.
 * @param m name of machine of which to set the light
 * @param light name of light color to set
 * @param state name of desired signal state of the light
 */
void
SPSComm::set_light(std::string &m, std::string &light, std::string &state)
{
  set_light(to_machine(m), to_light(light), to_signal_state(state));
}


/** Read puck ID via RFID.
 * @param m machine of which to read the puck
 * @param id upon returning true the read ID, unmodified otherwise
 * @return true if a puck was successfully read, false if there
 * is no puck under the reader or the communication was interrupted.
 */
bool
SPSComm::read_rfid(Machine m, uint32_t &id)
{
  const int addr = SPS_IN_REG_START_RFID + m * SPS_IN_REG_PER_RFID;
  uint16_t regs[3];
  if (modbus_read_registers(mb_, addr, SPS_IN_REG_PER_RFID, regs) != SPS_IN_REG_PER_RFID) {
    throw fawkes::Exception("Failed to read RFID registers: %s", modbus_strerror(errno));
  }

  if (regs[0] & SPS_RFID_HAS_PUCK) {
    id = regs[1] << 16 | regs[2];
    return true;
  } else {
    return false;
  }

}

/** Read puck IDs via RFID.
 * @return vector of IDs read from all of the RFID machines. If an
 * idea is 0xFFFFFFFF then no puck was placed below the sensor.
 */
std::vector<uint32_t>
SPSComm::read_rfids()
{
  const int addr = SPS_IN_REG_START_RFID;
  uint16_t regs[SPS_NUM_MACHINES * SPS_IN_REG_PER_RFID];
  if (modbus_read_registers(mb_, addr, SPS_NUM_MACHINES * SPS_IN_REG_PER_RFID, regs)
      != SPS_NUM_MACHINES * SPS_IN_REG_PER_RFID)
  {
    throw fawkes::Exception("Failed to read RFID registers: %s", modbus_strerror(errno));
  }

  std::vector<uint32_t> rv(SPS_NUM_MACHINES, 0xFFFFFFFF);
  for (unsigned int i = 0; i < SPS_NUM_MACHINES; ++i) {
    if (regs[i * SPS_IN_REG_PER_RFID] & SPS_RFID_HAS_PUCK) {
      // libmodbus has already swapped the bytes of the registers
      rv[i] = regs[i * SPS_IN_REG_PER_RFID + 1] << 16 | regs[i * SPS_IN_REG_PER_RFID + 2];
    }
  }

  return rv;
}


/** Reset all RFID registers.
 * Call this at the very beginning to avoid false values to be
 * written to a puck.
 */
void
SPSComm::reset_rfids()
{
  uint16_t *values = (uint16_t *)calloc(SPS_OUT_REG_PER_RFID * SPS_NUM_MACHINES,
					sizeof(uint16_t));
  if (modbus_write_registers(mb_, SPS_OUT_REG_START_RFID,
			     SPS_OUT_REG_PER_RFID * SPS_NUM_MACHINES, values) == -1)
  {
    free(values);
    throw fawkes::Exception("Failed to reset RFID registers: %s", modbus_strerror(errno));
  }
  free(values);
}


/** Write an ID to a puck using RFID.
 * @param m machine where to write
 * @param id ID to set on the puck
 */
void
SPSComm::write_rfid(Machine m, uint32_t id)
{
  //uint32_t old_id;
  //if (! read_rfid(m, old_id)) {
  //  throw fawkes::Exception("No puck under RFID sensor\n");
  //}

  const int out_addr = SPS_OUT_REG_START_RFID + m * SPS_OUT_REG_PER_RFID;
  uint16_t out_regs[3];
  out_regs[0] = SPS_RFID_WRITE_PUCK_ID;
  // libmodbus will swap the bytes of the register
  out_regs[1] = (id >> 16) & 0xFFFF;
  out_regs[2] = id & 0xFFFF;
  if (modbus_write_registers(mb_, out_addr, SPS_OUT_REG_PER_RFID, out_regs)
      != SPS_OUT_REG_PER_RFID)
  {
    throw fawkes::Exception("Failed to write RFID registers: %s", modbus_strerror(errno));
  }

  usleep(2000);

  const int in_addr = SPS_IN_REG_START_RFID + m * SPS_IN_REG_PER_RFID;
  uint16_t in_regs[3];
  do {
    if (modbus_read_registers(mb_, in_addr, SPS_IN_REG_PER_RFID, in_regs)
	!= SPS_IN_REG_PER_RFID)
    {
      throw fawkes::Exception("Failed to read RFID registers: %s", modbus_strerror(errno));
    }
    usleep(1000);
  } while (! (in_regs[0] & SPS_RFID_WRITE_DONE));


  out_regs[0] = SPS_RFID_RESET_WRITE;
  out_regs[1] = 0;
  out_regs[2] = 0;
  if (modbus_write_registers(mb_, out_addr, SPS_OUT_REG_PER_RFID, out_regs)
      != SPS_OUT_REG_PER_RFID)
  {
    throw fawkes::Exception("Failed to write RFID registers: %s", modbus_strerror(errno));
  }
}


/** Convert machine index to name.
 * @param index machine index
 * @return name of machine
 */ 
std::string &
SPSComm::index_to_name(uint32_t index)
{
  if (index < SPS_NUM_MACHINES) {
    return index_to_name_[index];
  } else {
    return index_to_name_[SPS_NUM_MACHINES];
  }
}


/** Convert string to Machine enum.
 * @param machine name of machine
 * @return corresponding Machine value
 */
SPSComm::Machine
SPSComm::to_machine(std::string &machine)
{
  std::map<std::string, Machine>::iterator m;
  if ((m = name_to_machine_.find(machine)) != name_to_machine_.end()) {
    return m->second;
  } else {
    throw fawkes::Exception("Unknown machine name '%s' requested", machine.c_str());
  }
}

/** Convert string to Light enum.
 * @param light name of light
 * @return corresponding Light value
 */
SPSComm::Light
SPSComm::to_light(std::string &light)
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
SPSComm::SignalState
SPSComm::to_signal_state(std::string &signal_state)
{
  std::map<std::string, SignalState>::iterator s;
  if ((s = name_to_signal_state_.find(signal_state)) != name_to_signal_state_.end()) {
    return s->second;
  } else {
    throw fawkes::Exception("Unknown signal state '%s' requested", signal_state.c_str());
  }
}



} // end of namespace llsfrb
