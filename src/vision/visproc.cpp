
/***************************************************************************
 *  visproc.cpp - LLSF RefBox vision processor
 *
 *  Created: Fri Jun 07 14:26:28 2013
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

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include "visproc.h"

#include <protobuf_comm/client.h>
#include <csignal>

#include "ssl_msgs/SslWrapper.pb.h"
#include <msgs/MachineReport.pb.h>

// defined in miliseconds
#define RECONNECT_TIMER_INTERVAL 1000

using namespace protobuf_comm;
using namespace llsf_msgs;

namespace llsfrb_visproc {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#if BOOST_ASIO_VERSION < 100601
static bool g_quit = false;
#endif

LLSFRefBoxVisionProcessor::LLSFRefBoxVisionProcessor()
  : quit_(false), reconnect_timer_(io_service_), try_reconnect_(true)
{
  client = new ProtobufStreamClient();
}


LLSFRefBoxVisionProcessor::~LLSFRefBoxVisionProcessor()
{
  quit_ = true;
  io_service_.stop();
  try_reconnect_ = false;
      
  reconnect_timer_.cancel();

  delete client;
  client = 0;
}


/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBoxVisionProcessor::handle_signal(int signum)
{
  reconnect_timer_.cancel();
  io_service_.stop();
}


/** Handle reconnect timer event.
 * @param error error code
 */
void
LLSFRefBoxVisionProcessor::handle_reconnect_timer(const boost::system::error_code& error)
{
  if (! error && try_reconnect_ && ! quit_) {
    client->async_connect("localhost", 4444);
  }
}

void
LLSFRefBoxVisionProcessor::client_connected()
{
  // put code here for what to do when connected to refbox
  //get_ssl_messages();
}

void
LLSFRefBoxVisionProcessor::client_disconnected(const boost::system::error_code &error)
{
  // automatically try to reconnect
  if (try_reconnect_) {
    reconnect_timer_.expires_from_now(boost::posix_time::milliseconds(RECONNECT_TIMER_INTERVAL));
    reconnect_timer_.async_wait(boost::bind(&LLSFRefBoxVisionProcessor::handle_reconnect_timer, this,
					    boost::asio::placeholders::error));
  }
}

void
LLSFRefBoxVisionProcessor::client_msg(uint16_t comp_id, uint16_t msg_type,
			    std::shared_ptr<google::protobuf::Message> msg)
{
  // put code here to do when receiving a message from the refbox
  // only handle the ones you are interested in

  // Example:
  // std::shared_ptr<llsf_msgs::GameState> g;
  // if ((g = std::dynamic_pointer_cast<llsf_msgs::GameState>(msg))) {
  //   // do something with message
  // }
}

#if BOOST_ASIO_VERSION < 100601
static void old_handle_signal(int signum)
{
  g_quit = true;
}
#endif

int
LLSFRefBoxVisionProcessor::run()
{
  // put initialization code here!
  sslclient.open(true);
  pass_ssl_messages();



  // Add messages you want to receive and process
  //MessageRegister & message_register = client->message_register();
  //message_register.add_message_type<llsf_msgs::GameState>();

  // Connect signals for network client
  client->signal_connected().connect(
    boost::bind(&LLSFRefBoxVisionProcessor::client_connected, this));
  client->signal_disconnected().connect(
    boost::bind(&LLSFRefBoxVisionProcessor::client_disconnected,
		this, boost::asio::placeholders::error));
  client->signal_received().connect(
    boost::bind(&LLSFRefBoxVisionProcessor::client_msg, this, _1, _2, _3));

  // Initial connect, will retry until connected asynchronously!
  client->async_connect("localhost", 4444);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBoxVisionProcessor::handle_signal, this,
				 boost::asio::placeholders::signal_number));
#else
  signal(SIGINT, old_handle_signal);
  signal(SIGTERM, old_handle_signal);
  while (! g_quit) {
    // Run the I/O main loop, the rest happens in the callbacks
    io_service_.run();
    usleep(500000);
  }
#endif

  // Run the I/O main loop, the rest happens in the callbacks
  io_service_.run();

  return 0;
}

void LLSFRefBoxVisionProcessor::pass_ssl_messages() {
  llsf_msgs::MachineReport report;
  llsf_msgs::MachineReportEntry *entry = report.add_machines();

  while(true) {
    if( sslclient.receive(incoming_packet) ) {
      if ( incoming_packet.has_detection() ) {
        SSLDetectionFrame detection;
        int number_pucks = detection.balls_size();
        for (int i = 0; i < number_pucks; i++) {
          printf("currently %d pucks visible!\n", i+1);
          entry->set_name("testname");
          entry->set_type("testtype");
        }
      }
      else {
        printf("nothing to see from up here...\n");
      }
      client->send(report);
    }
  }
}

} // end of namespace llsfrb_visproc

