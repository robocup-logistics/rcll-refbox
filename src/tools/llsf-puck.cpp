
/***************************************************************************
 *  llsf-puck.cpp - LLSF puck tool
 *
 *  Created: Thu Feb 14 09:40:43 2013
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
#include <boost/asio.hpp>
#include <utils/llsf/machines.h>

#include <memory>

using namespace llsf_sps;
using namespace fawkes;
using namespace llsf_utils;

#define TIMER_INTERVAL 50

typedef enum {
  READ,
  WRITE
} OpMode;

boost::asio::deadline_timer  *timer_;
#if BOOST_ASIO_VERSION >= 100601
boost::asio::signal_set      *signal_set_;
#endif
SPSComm                      *sps_;
unsigned int                  machine_;
uint32_t                      new_puck_id_;
OpMode                        op_mode_;

#if BOOST_ASIO_VERSION >= 100601
void
signal_handler(const boost::system::error_code& error, int signum)
{
  timer_->cancel();
}
#endif

void
handle_timer(const boost::system::error_code& error)
{
  if (! error) {
    uint32_t old_id = 0;
    if (sps_->read_rfid(machine_, old_id)) {
      if (op_mode_ == READ) {
	printf("Puck ID: %u\n", old_id);
      } else {
	printf("Writing new ID %u to puck (old was %u)...\n",
	       new_puck_id_, old_id);
	sps_->write_rfid(machine_, new_puck_id_);
	printf("Completed writing puck\n");
      }
#if BOOST_ASIO_VERSION >= 100601
      signal_set_->cancel();
#endif
    } else {
      timer_->expires_at(timer_->expires_at()
			 + boost::posix_time::milliseconds(TIMER_INTERVAL));
      timer_->async_wait(handle_timer);
    }
  }
}


void
usage(const char *progname)
{
  printf("Usage: %s [-m MACHINE_ID] read|write <puck-id>\n\n"
	 "read             read ID from puck under machine and print it\n"
	 "write <puck-id>  write new ID <puck-id> to puck, must be a number\n"
	 "-m MACHINE_ID    use this machine to write puck (M1 by default)\n"
	 "-t TEAM          Team, cyan or magenta (only 2014 assignment)\n"
	 "-Y Y             set assignment of year Y (2013 or 2014)\n",
	 progname);
}

int
main(int argc, char **argv)
{
  MachineAssignment machine_assignment = ASSIGNMENT_2014;

  ArgumentParser argp(argc, argv, "hm:Y:");

  if (argp.has_arg("h")) {
    usage(argv[0]);
    exit(0);
  }

  std::auto_ptr<llsfrb::Configuration> config(new llsfrb::YamlConfiguration(CONFDIR));
  config->load("config.yaml");

  printf("Connecting to SPS...\n");
  if (config->exists("/llsfrb/sps/hosts") && machine_assignment == ASSIGNMENT_2014) {
    sps_ = new SPSComm(config->get_strings("/llsfrb/sps/hosts"),
		       config->get_uint("/llsfrb/sps/port"));
  } else {
    sps_ = new SPSComm(config->get_string("/llsfrb/sps/host").c_str(),
		       config->get_uint("/llsfrb/sps/port"));
  }

  sps_->reset_lights();
  sps_->reset_rfids();

  machine_ = 0;


  if (argp.has_arg("Y")) {
    std::string year = argp.arg("Y");
    if (year == "2013") {
      machine_assignment = ASSIGNMENT_2013;
    } else if (year == "2014") {
      machine_assignment = ASSIGNMENT_2014;
    } else {
      printf("Invalid assignment, must be 2013 or 2014");
      usage(argv[0]);
      exit(-1);
    }
  }

  if (argp.has_arg("m")) {
    std::string machine_str = argp.arg("m");
    machine_ = to_machine(machine_str, machine_assignment);
  }

  if (argp.num_items() == 0) {
    usage(argv[0]);
    delete sps_;
    exit(1);
  }

  std::string arg_opmode = argp.items()[0];
  if (arg_opmode == "read") {
    op_mode_ = READ;
  } else if (arg_opmode == "write") {
    op_mode_ = WRITE;
    if (argp.num_items() != 2) {
      usage(argv[0]);
      delete sps_;
      exit(2);
    }
    new_puck_id_ = argp.parse_item_int(1);
    if (new_puck_id_ == 0) {
      printf("Invalid ID passed\n");
      exit(3);
    }
  } else {
    printf("Unknown operation\n");
    usage(argv[0]);
    delete sps_;
    exit(4);    
  }

  boost::asio::io_service io_service;

  printf("Waiting for puck to appear under machine...\n");
  timer_ = new boost::asio::deadline_timer(io_service);
  timer_->expires_from_now(boost::posix_time::milliseconds(TIMER_INTERVAL));
  timer_->async_wait(handle_timer);


#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  signal_set_ = new boost::asio::signal_set(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signal_set_->async_wait(signal_handler);
#endif

  io_service.run();

  delete timer_;
  delete sps_;
}
