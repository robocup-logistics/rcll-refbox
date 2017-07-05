
/***************************************************************************
 *  rcll-set-lights.cpp - set MPS lights
 *
 *  Created: Mon Mar 21 17:18:21 2016
 *  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
 *                  2017  Tobias Neumann
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

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <config/yaml.h>

#include <utils/system/argparser.h>

#include <thread>         // std::thread
#include <mutex>          // std::mutex
#include <iostream>

using namespace fawkes;

boost::asio::io_service io_service_;

static bool quit = false;

static std::string machine_name_;
//static int id_last_ = 0;

static std::shared_ptr<std::thread> mps_msg_thread_;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
    io_service_.stop();
  }
}

void
input_loop()
{
  do {
    std::cout << std::endl
              << std::endl
              << "What do you want to send to the SPS? Options:" << std::endl
              << "Set light, press         l" << std::endl
              << "Move the conveyor, press c" << std::endl
              << "Reset SPS, press         r" << std::endl
              << std::endl;

    std::string input_line = "";
    std::cin >> input_line;
    if ( ! input_line.empty() ) {
      switch (input_line.at(0)) {
        case 'l':
          std::cout << "do light specific stuff" << std::endl;
          break;
        case 'c':
          std::cout << "do conveyor specific stuff" << std::endl;
          break;
        case 'r':
          std::cout << "rest SPS" << std::endl;
          break;
      }
    }

    sleep(1);
  } while ( ! quit );
}

void
usage(const char *progname)
{
  printf("Usage: %s -m <machine-name>\n", progname);
}


int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "m:R");

  if ( ! (argp.has_arg("m")) ) {
    usage(argv[0]);
    exit(1);
  }
  machine_name_ = argp.arg("m");
  std::cout << machine_name_ << std::endl;

//  llsfrb::YamlConfiguration *config_ = new llsfrb::YamlConfiguration(CONFDIR);
//  config_->load("config.yaml");
//  std::string cfg_prefix = std::string("/llsfrb/mps/stations/");
//  std::string host = config_->get_string( (cfg_prefix + machine_name_ + "/host").c_str() );
//  unsigned int port = config_->get_uint( (cfg_prefix + machine_name_ + "/port").c_str() );

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  mps_msg_thread_ = std::shared_ptr<std::thread>(new std::thread(input_loop));

  do {
    io_service_.run();
    io_service_.reset();
  } while (! quit);
}
