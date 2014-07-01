
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

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

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
#include <msgs/RobotInfo.pb.h>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool quit = false;
static boost::asio::deadline_timer *timer_ = NULL;
std::string name_;
Team team_color_;
std::string team_name_;
unsigned long seq_ = 0;
ProtobufBroadcastPeer *peer_public_ = NULL;
ProtobufBroadcastPeer *peer_team_ = NULL;
bool crypto_setup_ = false;

llsfrb::Configuration *config_;

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
    printf("Detected robot: %u %s:%s (seq %lu)\n",
#else
    printf("Detected robot: %u %s:%s (seq %llu)\n",
#endif
	   b->number(), b->team_name().c_str(), b->peer_name().c_str(), b->seq());
  }

  std::shared_ptr<GameState> gs;
  if ((gs = std::dynamic_pointer_cast<GameState>(msg))) {
    int hour = gs->game_time().sec() / 3600;
    int min  = (gs->game_time().sec() - hour * 3600) / 60;
    int sec  = gs->game_time().sec() - hour * 3600 - min * 60;

#if __WORDSIZE == 64
    printf("GameState received:  %02i:%02i:%02i.%02ld  %s %s  %u:%u points, %s vs. %s\n",
#else
    printf("GameState received:  %02i:%02i:%02i.%02lld  %s %s  %u:%u points, %s vs. %s\n",
#endif
	   hour, min, sec, gs->game_time().nsec() / 1000000,
	   llsf_msgs::GameState::Phase_Name(gs->phase()).c_str(),
	   llsf_msgs::GameState::State_Name(gs->state()).c_str(),
	   gs->points_cyan(), gs->points_magenta(),
	   gs->team_cyan().c_str(), gs->team_magenta().c_str());

    if (team_name_ == gs->team_cyan() || team_name_ == gs->team_magenta()) {
      if (team_name_ == gs->team_cyan() && team_color_ != CYAN) {
	printf("WARNING: sending as magenta, but our team is announced as cyan by refbox!\n");
      } else if (team_name_ == gs->team_magenta() && team_color_ != MAGENTA) {
	printf("WARNING: sending as cyan, but our team is announced as magenta by refbox!\n");
      }
      if (! crypto_setup_) {
	crypto_setup_ = true;

	std::string crypto_key = "", cipher = "aes-128-cbc";
	try {
	  crypto_key = config_->get_string(("/llsfrb/game/crypto-keys/" + team_name_).c_str());
	  printf("Set crypto key to %s (cipher %s)\n", crypto_key.c_str(), cipher.c_str());
	  peer_team_->setup_crypto(crypto_key, cipher);
	} catch (Exception &e) {
	  printf("No encryption key configured for team, not enabling crypto");
	}
      }
    } else if (crypto_setup_) {
      printf("Our team is not set, training game? Disabling crypto.\n");
      crypto_setup_ = false;
      peer_team_->setup_crypto("", "");
    }
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
      printf("  %-3s|%2s|%s @ (%f, %f, %f)\n",
	     m.name().c_str(), m.type().substr(0, 2).c_str(),
	     Team_Name(m.team_color()).substr(0, 2).c_str(),
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

  std::shared_ptr<RobotInfo> ri;
  if ((ri = std::dynamic_pointer_cast<RobotInfo>(msg))) {
    printf("Robot Info received:\n");
    for (int i = 0; i < ri->robots_size(); ++i) {
      const llsf_msgs::Robot &r = ri->robots(i);

      const llsf_msgs::Time &time = r.last_seen();

      boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
      boost::posix_time::ptime last_seen =
	boost::posix_time::from_time_t(time.sec())
	+ boost::posix_time::nanoseconds(time.nsec());


      boost::posix_time::time_duration const last_seen_ago_td = now - last_seen;
      float last_seen_ago = last_seen_ago_td.total_milliseconds() / 1000.f;
      
      printf("  %u %s/%s @ %s: state %s, last seen %f sec ago  Maint cyc: %u  rem: %f\n",
	     r.number(), r.name().c_str(), r.team().c_str(), r.host().c_str(),
	     llsf_msgs::RobotState_Name(r.state()).substr(0,3).c_str(),
	     last_seen_ago, r.maintenance_cycles(), r.maintenance_time_remaining());
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

    Pose2D *pose = signal->mutable_pose();
    pose->set_x(1.0);
    pose->set_y(2.0);
    pose->set_ori(3.0);

    Time *pose_time = pose->mutable_timestamp();
    pose_time->set_sec(4);
    pose_time->set_nsec(5);

    signal->set_number(1);
    signal->set_peer_name(name_);
    signal->set_team_name(team_name_);
    signal->set_team_color(team_color_);
    signal->set_seq(++seq_);
    peer_team_->send(signal);

    timer_->expires_at(timer_->expires_at()
		      + boost::posix_time::milliseconds(2000));
    timer_->async_wait(handle_timer);
  }

}


int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "T:");

  if (argp.num_items() != 2) {
    printf("Usage: %s <name> <team>\n", argv[0]);
    exit(1);
  }

  name_ = argp.items()[0];
  team_name_ = argp.items()[1];

  team_color_ = CYAN;
  if (argp.has_arg("T")) {
    std::string team_str = argp.arg("T");
    if (team_str == "cyan") {
      team_color_ = CYAN;
    } else if (team_str == "magenta") {
      team_color_ = MAGENTA;
    } else {
      printf("Unknown team value, using cyan\n");
    }
  }

  config_ = new llsfrb::YamlConfiguration(CONFDIR);
  config_->load("config.yaml");

  if (config_->exists("/llsfrb/comm/public-peer/send-port") &&
      config_->exists("/llsfrb/comm/public-peer/recv-port") )
  {
    peer_public_ = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
					     config_->get_uint("/llsfrb/comm/public-peer/recv-port"),
					     config_->get_uint("/llsfrb/comm/public-peer/send-port"));
  } else {
    peer_public_ = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
					     config_->get_uint("/llsfrb/comm/public-peer/port"));
  }

  MessageRegister & message_register = peer_public_->message_register();
  message_register.add_message_type<BeaconSignal>();
  message_register.add_message_type<OrderInfo>();
  message_register.add_message_type<GameState>();
  message_register.add_message_type<VersionInfo>();
  message_register.add_message_type<ExplorationInfo>();
  message_register.add_message_type<MachineInfo>();
  message_register.add_message_type<MachineReportInfo>();
  message_register.add_message_type<RobotInfo>();

  std::string cfg_prefix =
    std::string("/llsfrb/comm/") +
    ((team_color_ == CYAN) ? "cyan" : "magenta") + "-peer/";

  /*
  // better to this dynamically be reacting to the public GameState
  // this way you can also play unencrypted training games
  std::string crypto_key = "", cipher = "aes-128-cbc";
  try {
    crypto_key = config_->get_string(("/llsfrb/game/crypto-keys/" + team_name_).c_str());
  } catch (Exception &e) {
    printf("No encryption key configured for team, not enabling crypto");
  }
  */

  if (config_->exists((cfg_prefix + "send-port").c_str()) &&
      config_->exists((cfg_prefix + "recv-port").c_str()) )
  {
    peer_team_ = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
					   config_->get_uint((cfg_prefix + "recv-port").c_str()),
					   config_->get_uint((cfg_prefix + "send-port").c_str()),
					   &message_register /*, crypto_key, cipher*/);
  } else {
    peer_team_ = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
					   config_->get_uint((cfg_prefix + "port").c_str()),
					   &message_register/*, crypto_key, cipher*/);
  }

  boost::asio::io_service io_service;

  peer_public_->signal_received().connect(handle_message);
  peer_public_->signal_recv_error().connect(handle_recv_error);
  peer_public_->signal_send_error().connect(handle_send_error);

  peer_team_->signal_received().connect(handle_message);
  peer_team_->signal_recv_error().connect(handle_recv_error);
  peer_team_->signal_send_error().connect(handle_send_error);

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
  delete peer_team_;
  delete peer_public_;
  delete config_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
