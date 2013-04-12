
/***************************************************************************
 *  llsf-fake-robot.cpp - fake a robot
 *
 *  Created: Fri Feb 22 11:55:51 2013
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

#include <protobuf_comm/peer.h>
#include <utils/system/argparser.h>

#include <msgs/BeaconSignal.pb.h>
#include <msgs/OrderInfo.pb.h>
#include <msgs/GameState.pb.h>
#include <msgs/VersionInfo.pb.h>
#include <msgs/ExplorationInfo.pb.h>
#include <msgs/MachineInfo.pb.h>
#include <msgs/MachineReport.pb.h>


#include <boost/asio.hpp>
#include <boost/date_time.hpp>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool quit = false;
static boost::asio::deadline_timer *timer_ = NULL;
std::string name_;
std::string team_;
unsigned long seq_ = 0;
ProtobufBroadcastPeer *peer_ = NULL;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;

    if (timer_) {
      timer_->cancel();
    }
  }
}

void
handle_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg)
{
  printf("Receive error from %s:%u: %s\n",
	 endpoint.address().to_string().c_str(), endpoint.port(), msg.c_str());
}

void
handle_send_error(std::string msg)
{
  printf("Send error: %s\n", msg.c_str());
}

void
handle_message(boost::asio::ip::udp::endpoint &sender,
	       uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<BeaconSignal> b;
  if ((b = std::dynamic_pointer_cast<BeaconSignal>(msg))) {
#if __WORDSIZE == 64
    printf("Detected robot: %s:%s (seq %lu)\n",
#else
    printf("Detected robot: %s:%s (seq %llu)\n",
#endif
	   b->team_name().c_str(), b->peer_name().c_str(), b->seq());
  }

  std::shared_ptr<GameState> gs;
  if ((gs = std::dynamic_pointer_cast<GameState>(msg))) {
    int hour = gs->game_time().sec() / 3600;
    int min  = (gs->game_time().sec() - hour * 3600) / 60;
    int sec  = gs->game_time().sec() - hour * 3600 - min * 60;

#if __WORDSIZE == 64
    printf("GameState received:  %02i:%02i:%02i.%02ld  %s %s  %u points\n",
#else
    printf("GameState received:  %02i:%02i:%02i.%02lld  %s %s  %u points\n",
#endif
	   hour, min, sec, gs->game_time().nsec() / 1000000,
	   llsf_msgs::GameState::Phase_Name(gs->phase()).c_str(),
	   llsf_msgs::GameState::State_Name(gs->state()).c_str(),
	   gs->points());
  }

  std::shared_ptr<OrderInfo> oi;
  if ((oi = std::dynamic_pointer_cast<OrderInfo>(msg))) {
    printf("Order Info received:\n");
    for (int i = 0; i < oi->orders_size(); ++i) {
      const llsf_msgs::Order &o = oi->orders(i);
      unsigned int begin_min = o.delivery_period_begin() / 60;
      unsigned int begin_sec = o.delivery_period_begin() - begin_min * 60;
      unsigned int end_min = o.delivery_period_end() / 60;
      unsigned int end_sec = o.delivery_period_end() - end_min * 60;

      printf("  %u: %u/%u of %s from %02u:%02u to %02u:%02u at gate %s\n", o.id(),
	     o.quantity_delivered(), o.quantity_requested(),
	     llsf_msgs::Order::ProductType_Name(o.product()).c_str(),
	     begin_min, begin_sec, end_min, end_sec,
	     llsf_msgs::Order::DeliveryGate_Name(o.delivery_gate()).c_str());
    }
  }

  std::shared_ptr<VersionInfo> vi;
  if ((vi = std::dynamic_pointer_cast<VersionInfo>(msg))) {
    printf("VersionInfo received: %s\n", vi->version_string().c_str());
  }

  std::shared_ptr<ExplorationInfo> ei;
  if ((ei = std::dynamic_pointer_cast<ExplorationInfo>(msg))) {
    printf("ExplorationInfo received:\n");
    for (int i = 0; i < ei->signals_size(); ++i) {
      const ExplorationSignal &es = ei->signals(i);
      printf("  Machine type %s assignment:", es.type().c_str());
      for (int j = 0; j < es.lights_size(); ++j) {
	const LightSpec &lspec = es.lights(j);
	printf(" %s=%s", LightColor_Name(lspec.color()).c_str(),
	       LightState_Name(lspec.state()).c_str());
      }
      printf("\n");
    }
    printf("  --\n");
    for (int i = 0; i < ei->machines_size(); ++i) {
      const ExplorationMachine &em = ei->machines(i);
      printf("  Machine %s at (%f, %f, %f)\n", em.name().c_str(),
	     em.pose().x(), em.pose().y(), em.pose().ori());
    }
  }

  std::shared_ptr<MachineInfo> mi;
  if ((mi = std::dynamic_pointer_cast<MachineInfo>(msg))) {
    printf("MachineInfo received:\n");
    for (int i = 0; i < mi->machines_size(); ++i) {
      const Machine &m = mi->machines(i);
      const Pose2D &p = m.pose();
      printf("  %-3s|%2s @ (%f, %f, %f)\n",
	     m.name().c_str(), m.type().substr(0, 2).c_str(),
	     p.x(), p.y(), p.ori());
    }
  }

  std::shared_ptr<MachineReportInfo> mri;
  if ((mri = std::dynamic_pointer_cast<MachineReportInfo>(msg))) {
    printf("MachineReportInfo received:\n");
    if (mri->reported_machines_size() > 0) {
      printf("  Reported machines:");
      for (int i = 0; i < mri->reported_machines_size(); ++i) {
	printf(" %s", mri->reported_machines(i).c_str());
      }
      printf("\n");
    } else {
      printf("  no machines reported, yet\n");
    }
  }
}


void
handle_timer(const boost::system::error_code& error)
{
  if (! error) {
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
    std::shared_ptr<BeaconSignal> signal(new BeaconSignal());
    Time *time = signal->mutable_time();
    boost::posix_time::time_duration const since_epoch =
      now - boost::posix_time::from_time_t(0);

    time->set_sec(static_cast<google::protobuf::int64>(since_epoch.total_seconds()));
    time->set_nsec(
      static_cast<google::protobuf::int64>(since_epoch.fractional_seconds() * 
					   (1000000000/since_epoch.ticks_per_second())));

    signal->set_peer_name(name_);
    signal->set_team_name(team_);
    signal->set_seq(++seq_);
    peer_->send(signal);

    timer_->expires_at(timer_->expires_at()
		      + boost::posix_time::milliseconds(2000));
    timer_->async_wait(handle_timer);
  }

}


int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "");

  if (argp.num_items() != 2) {
    printf("Usage: %s <name> <team>\n", argv[0]);
    exit(1);
  }

  name_ = argp.items()[0];
  team_ = argp.items()[1];

  llsfrb::Configuration *config = new llsfrb::YamlConfiguration(CONFDIR);
  config->load("config.yaml");

  if (config->exists("/llsfrb/comm/peer-send-port") &&
      config->exists("/llsfrb/comm/peer-recv-port") )
  {
    peer_ = new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
				      config->get_uint("/llsfrb/comm/peer-recv-port"),
				      config->get_uint("/llsfrb/comm/peer-send-port"));
  } else {
    peer_ = new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
				      config->get_uint("/llsfrb/comm/peer-port"));
  }

  boost::asio::io_service io_service;

  MessageRegister & message_register = peer_->message_register();
  message_register.add_message_type<BeaconSignal>();
  message_register.add_message_type<OrderInfo>();
  message_register.add_message_type<GameState>();
  message_register.add_message_type<VersionInfo>();
  message_register.add_message_type<ExplorationInfo>();
  message_register.add_message_type<MachineInfo>();
  message_register.add_message_type<MachineReportInfo>();

  peer_->signal_received().connect(handle_message);
  peer_->signal_recv_error().connect(handle_recv_error);
  peer_->signal_send_error().connect(handle_send_error);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  timer_ = new boost::asio::deadline_timer(io_service);
  timer_->expires_from_now(boost::posix_time::milliseconds(2000));
  timer_->async_wait(handle_timer);

  do {
    io_service.run();
    io_service.reset();
  } while (! quit);

  delete timer_;
  delete peer_;
  delete config;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
