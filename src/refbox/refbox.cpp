
/***************************************************************************
 *  refbox.cpp - LLSF RefBox main program
 *
 *  Created: Thu Feb 07 11:04:17 2013
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

#include "refbox.h"
#include "sps_comm.h"

#include <config/yaml.h>
#include <clipsmm.h>

#include <boost/bind.hpp>

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class LLSFRefBox "refbox.h"
 * LLSF referee box main application.
 * @author Tim Niemueller
 */ 

/** Constructor.
 * @param argc number of arguments passed
 * @param argv array of arguments
 */
LLSFRefBox::LLSFRefBox(int argc, char **argv)
  : timer_(io_service_)
{

  config_ = new YamlConfiguration(CONFDIR);
  config_->load("config.yaml");

  try {
    cfg_timer_interval_ = config_->get_uint("/llsfrb/clips/timer-interval");
  } catch (fawkes::Exception &e) {
    delete config_;
    throw;
  }

  try {
    printf("Connectiong to SPS\n");
    sps_ = NULL;
    if (config_->get_bool("/llsfrb/sps/enable")) {
      sps_ = new SPSComm(config_->get_string("/llsfrb/sps/host").c_str(),
			 config_->get_uint("/llsfrb/sps/port"));

      sps_->reset_lights();
      sps_->reset_rfids();
      sps_->test_lights();
    }
  } catch (fawkes::Exception &e) {
    printf("Cannot connect to SPS, running without\n");
    delete sps_;
    sps_ = NULL;
  }

  printf("Creating CLIPS environment\n");
  clips_ = new CLIPS::Environment();
}

/** Destructor. */
LLSFRefBox::~LLSFRefBox()
{
  timer_.cancel();
  delete config_;
  delete sps_;
  delete clips_;
}


/** Start the timer for another run. */
void
LLSFRefBox::start_timer()
{
  timer_.expires_from_now(boost::posix_time::milliseconds(cfg_timer_interval_));
  timer_.async_wait(boost::bind(&LLSFRefBox::handle_timer, this,
				boost::asio::placeholders::error));
}

/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBox::handle_timer(const boost::system::error_code& error)
{
  if (! error) {
    start_timer();
  }
}


/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBox::handle_signal(const boost::system::error_code& error, int signum)
{
  timer_.cancel();
}

/** Run the application.
 * @return return code, 0 if no error, error code otherwise
 */
int
LLSFRefBox::run()
{
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBox::handle_signal, this,
				 boost::asio::placeholders::error,
				 boost::asio::placeholders::signal_number));

  start_timer();
  io_service_.run();
  return 0;
}

} // end of namespace llsfrb
