
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

#include <llsf_sps/sps_comm.h> 
#include <boost/asio.hpp>

#include <memory>

using namespace llsf_sps;
using namespace fawkes;

boost::asio::signal_set      *signal_set_;
SPSComm                      *sps_;

void
signal_handler(const boost::system::error_code& error, int signum)
{
}

int
main(int argc, char **argv)
{
  //ArgumentParser argp(argc, argv, "m:");

  std::auto_ptr<llsfrb::Configuration> config(new llsfrb::YamlConfiguration(CONFDIR));
  config->load("config.yaml");

  printf("Connecting to SPS...\n");
  sps_ = new SPSComm(config->get_string("/llsfrb/sps/host").c_str(),
		     config->get_uint("/llsfrb/sps/port"));
  sps_->reset_lights();
  sps_->reset_rfids();

  sps_->test_lights();
  sps_->set_light(SPSComm::R2, SPSComm::LIGHT_RED, SPSComm::SIGNAL_ON);
  sps_->set_light(SPSComm::R2, SPSComm::LIGHT_YELLOW, SPSComm::SIGNAL_ON);
  sps_->set_light(SPSComm::R2, SPSComm::LIGHT_GREEN, SPSComm::SIGNAL_ON);

  boost::asio::io_service io_service;

  // Construct a signal set registered for process termination.
  signal_set_ = new boost::asio::signal_set(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signal_set_->async_wait(signal_handler);

  io_service.run();

  delete sps_;
}
