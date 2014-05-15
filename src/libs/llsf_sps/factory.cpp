
/***************************************************************************
 *  factory.cpp - LLSF RefBox Machine Communication Factory
 *
 *  Created: Mon Apr 21 12:32:32 2014
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

#include <llsf_sps/factory.h>
#include <llsf_sps/sps_comm.h>
#include <llsf_sps/arduino_comm.h>
#include <config/config.h>
#include <logging/logger.h>
#include <utils/llsf/machines.h>

using namespace llsf_utils;

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


/** Hidden constructor. */
MachineCommunicationFactory::MachineCommunicationFactory()
{
}

/** Create a machine communicator instance.
 * @param config configuration from which to read what machine to instantiate
 * @param discover true to discover all machines (for communicators that support this),
 * or false if you want to communicate only with specific machines (can speed up
 * initialization for some communicators.
 * @param logger optional logger for error messages
 * @return machine communication instance
 */
MachineCommunication *
MachineCommunicationFactory::create(llsfrb::Configuration *config, bool discover,
				    llsfrb::Logger *logger)
{
  std::string mtype = config->get_string("/llsfrb/sps/type");

  llsf_utils::MachineAssignment cfg_machine_assignment = ASSIGNMENT_2014;
  try {
    std::string m_ass_str = config->get_string("/llsfrb/game/machine-assignment");
    if (m_ass_str == "2013") {
      cfg_machine_assignment = ASSIGNMENT_2013;
    } else if (m_ass_str == "2014") {
      cfg_machine_assignment = ASSIGNMENT_2014;
    } else {
      if (logger) {
	logger->log_warn("RefBox", "Invalid machine assignment '%s', using 2014",
			 m_ass_str.c_str());
      }
      cfg_machine_assignment = ASSIGNMENT_2014;
    }
  } catch (fawkes::Exception &e) {} // ignored, use default

  if (mtype == "SPS") {
    if (config->exists("/llsfrb/sps/hosts") && cfg_machine_assignment == ASSIGNMENT_2014) {
      return new SPSComm(config->get_strings("/llsfrb/sps/hosts"),
			 config->get_uint("/llsfrb/sps/port"));
    } else {
      return new SPSComm(config->get_string("/llsfrb/sps/host").c_str(),
			 config->get_uint("/llsfrb/sps/port"));
    }
  } else if (mtype == "ArduinoXBee") {
    return new ArduinoXBeeComm(config->get_string("/llsfrb/sps/device").c_str(), discover);
  } else {
    throw fawkes::Exception("Unknown machine communication type");
  }
}


} // end of namespace llsf_sps
