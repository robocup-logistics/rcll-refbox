
/***************************************************************************
 *  llsf-signal-ctrl.cpp - LLSF signal control tool
 *
 *  Created: Fri Mar 01 11:04:41 2013
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

#include <config/yaml.h>

#include <utils/system/argparser.h>
#include <llsf_sps/sps_comm.h>
#include <utils/llsf/machines.h>

#include <unistd.h>

using namespace llsf_sps;
using namespace fawkes;
using namespace llsf_utils;

void
usage(const char *progname)
{
  printf("Usage: %s -m MACHINE_ID -r S -g S -b S \n\n"
	 "-R               reset all lights\n"
	 "-m MACHINE_ID    use this machine to write puck\n"
	 "-r S             set red light to state S (ON, OFF, or BLINK)\n"
	 "-g S             set green light to state S (ON, OFF, or BLINK)\n"
	 "-y S             set yellow light to state S (ON, OFF, or BLINK)\n"
	 "-Y Y             set assignment of year Y (2013 or 2014)\n",
	 progname);
}


int
main(int argc, char **argv)
{
  MachineAssignment machine_assignment = ASSIGNMENT_2014;

  ArgumentParser argp(argc, argv, "hRm:r:g:y:Y:");

  if (argp.has_arg("h")) {
    usage(argv[0]);
    exit(0);
  }

  std::auto_ptr<llsfrb::Configuration> config(new llsfrb::YamlConfiguration(CONFDIR));
  config->load("config.yaml");

  //printf("Connecting to SPS...\n");  
  SPSComm *sps_;
  if (config->exists("/llsfrb/sps/hosts") && machine_assignment == ASSIGNMENT_2014) {
    sps_ = new SPSComm(config->get_strings("/llsfrb/sps/hosts"),
		       config->get_uint("/llsfrb/sps/port"));
  } else {
    sps_ = new SPSComm(config->get_string("/llsfrb/sps/host").c_str(),
		       config->get_uint("/llsfrb/sps/port"));
  }

  //sps_->reset_lights();
  sps_->reset_rfids();

  unsigned int         machine  = 0;
  SPSComm::SignalState red_s    = SPSComm::SIGNAL_OFF;
  SPSComm::SignalState green_s  = SPSComm::SIGNAL_OFF;
  SPSComm::SignalState yellow_s = SPSComm::SIGNAL_OFF;

  if (argp.has_arg("Y")) {
    std::string year = argp.arg("Y");
    if (year == "2013") {
      machine_assignment = ASSIGNMENT_2013;
    } else if (year == "2014") {
      machine_assignment = ASSIGNMENT_2014;
    } else {
      printf("Invalid assignment, must be 2013 or 2014\n");
      usage(argv[0]);
      exit(-1);
    }
  }

  if (argp.has_arg("R")) {
    sps_->reset_lights();
  }

  if (argp.has_arg("m")) {
    std::string machine_str = argp.arg("m");
    machine = to_machine(machine_str, machine_assignment);
  }

  if (argp.has_arg("r")) {
    std::string signal_state = argp.arg("r");
    red_s = sps_->to_signal_state(signal_state);
  }

  if (argp.has_arg("g")) {
    std::string signal_state = argp.arg("g");
    green_s = sps_->to_signal_state(signal_state);
  }

  if (argp.has_arg("y")) {
    std::string signal_state = argp.arg("y");
    yellow_s = sps_->to_signal_state(signal_state);
  }


  //sps_->test_lights();
  sps_->set_light(machine, SPSComm::LIGHT_RED, red_s);
  sps_->set_light(machine, SPSComm::LIGHT_YELLOW, yellow_s);
  sps_->set_light(machine, SPSComm::LIGHT_GREEN, green_s);

  usleep(500000);

  delete sps_;
}
