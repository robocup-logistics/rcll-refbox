
/***************************************************************************
 *  llsf-sync-print.cpp - print sync configuration received from refbox
 *
 *  Created: Tue Jun 11 17:20:03 2013
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

#include <protobuf_comm/client.h>
#include <utils/system/argparser.h>

#include <msgs/Sync.pb.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool quit = false;
ProtobufStreamClient *client_ = NULL;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
  }
}

void
handle_connected()
{
  printf("Sending InitiateSync\n");
  InitiateSync init_msg;
  client_->send(init_msg);
}

std::string
format_time(float time)
{
  int hour = (int)truncf(time) / 3600;
  int min  = (int)truncf(time - (hour * 3600)) / 60;
  int sec  = time - (hour * 3600) - (min * 60);
  char *tmp;
  if (hour > 0) {
    if (asprintf(&tmp, "%02d:%02d:%02d", hour, min, sec) != -1) {
      std::string rv = tmp;
      free(tmp);
      return rv;
    }
  } else {
    if (asprintf(&tmp, "%02d:%02d", min, sec) != -1) {
      std::string rv = tmp;
      free(tmp);
      return rv;
    }
  }

  return "";
}

void
handle_message(uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<SyncInfo> s;
  if ((s = std::dynamic_pointer_cast<SyncInfo>(msg))) {
    printf("Received SyncInfo:\n\n");

    for (int i = 0; i < s->machines_size(); ++i) {
      const MachineTypeSpec &m = s->machines(i);
      printf("Machine  %-3s of type %s\n", m.name().c_str(), m.type().c_str());
    }


    for (int i = 0; i < s->exploration_lights_size(); ++i) {
      const llsf_msgs::MachineLightSpec &mls = s->exploration_lights(i);

      printf("Light Code of %s: ", mls.machine_type().c_str());
      for (int j = 0; j < mls.lights_size(); ++j) {
	const LightSpec &l = mls.lights(j);
	printf(" %s-%s", LightColor_Name(l.color()).c_str(),
	       LightState_Name(l.state()).c_str());
      }
      printf("\n");
    }

    for (int i = 0; i < s->proc_times_size(); ++i) {
      const llsf_msgs::MachineProcTime &mpt = s->proc_times(i);
      printf("Proc time of %s: %u sec\n",
	     mpt.machine_type().c_str(), mpt.proc_time());
    }

    for (int i = 0; i < s->orders_size(); ++i) {
      const llsf_msgs::Order &o = s->orders(i);
      unsigned int begin_min = o.delivery_period_begin() / 60;
      unsigned int begin_sec = o.delivery_period_begin() - begin_min * 60;
      unsigned int end_min = o.delivery_period_end() / 60;
      unsigned int end_sec = o.delivery_period_end() - end_min * 60;

      printf("Order%s %2u: %2u of %s from %02u:%02u to %02u:%02u at gate %s\n",
	     o.late_order() ? "/LO" : "   ", o.id(),
	     o.quantity_requested(),
	     llsf_msgs::Order::ProductType_Name(o.product()).c_str(),
	     begin_min, begin_sec, end_min, end_sec,
	     llsf_msgs::Order::DeliveryGate_Name(o.delivery_gate()).c_str());
    }

    for (int i = 0; i < s->down_times_size(); ++i) {
      const llsf_msgs::MachineTimeSpec &mt = s->down_times(i);
      printf("Down time  %s from %s to %s\n", mt.machine_name().c_str(),
	     format_time(mt.gt_from()).c_str(), format_time(mt.gt_to()).c_str());
    }

    for (int i = 0; i < s->delivery_gates_size(); ++i) {
      const llsf_msgs::MachineTimeSpec &mt = s->delivery_gates(i);
      printf("Delivery @  %s from %s to %s\n", mt.machine_name().c_str(),
	     format_time(mt.gt_from()).c_str(), format_time(mt.gt_to()).c_str());
    }
  }
}


int
main(int argc, char **argv)
{
  client_ = new ProtobufStreamClient();

  boost::asio::io_service io_service;

  MessageRegister & message_register = client_->message_register();
  message_register.add_message_type<SyncInfo>();

  client_->signal_received().connect(handle_message);
  client_->signal_connected().connect(handle_connected);
  client_->async_connect("localhost", 4444);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  do {
    io_service.run();
    io_service.reset();
  } while (! quit);

  delete client_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
