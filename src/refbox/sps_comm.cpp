
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

#include "sps_comm.h"

#include <modbus.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>

#include <core/exception.h>
#include <core/exceptions/software.h>

namespace llsfrb {
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
}


/** Destructor. */
SPSComm::~SPSComm()
{
  if (mb_) {
    modbus_close(mb_);
    modbus_free(mb_);
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
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_BLINK);
    }
  }
  usleep(2000000);
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_ON);
    }
  }
  usleep(500000);

  for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
    reset_lights();
    for (int m = M_BEGIN; m < M_END; ++m) {
      set_light((Machine)m, (Light)l, SIGNAL_ON);
    }
    usleep(250000);
  }

  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_ON);
    }
  }
  usleep(1000000);
  for (int m = M_BEGIN; m < M_END; ++m) {
    for (int l = LIGHT_BEGIN; l < LIGHT_END; ++l) {
      set_light((Machine)m, (Light)l, SIGNAL_OFF);
    }
    usleep(150000);
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
    id = htonl(regs[1] << 16 | regs[2]);
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
      rv[i] = htonl(regs[i * SPS_IN_REG_PER_RFID + 1] << 16 |
		    regs[i * SPS_IN_REG_PER_RFID + 2]);
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
  out_regs[1] = id >> 16;
  out_regs[2] = id & 0xffff;
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


} // end of namespace llsfrb
