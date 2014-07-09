/***************************************************************************
 *  llsf-view2.cpp - RoboView2 comuunication between ProboBuf and RoboView2
 *
 *  Created: Sat April 13 20:03:00 2013
 *  Copyright  2013  Wataru UEMURA [friede.elec.ryukoku.ac.jp]
 ****************************************************************************/
 
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

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>

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

#include "llsf-view2.h"

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool quit = false;
static boost::asio::deadline_timer *timer_ = NULL;
std::string name_[4];
Team team_;
std::string team_name_;
unsigned long seq_[3] = {0, 0, 0};
// ProtobufBroadcastPeer *peer_[4] = {NULL, NULL, NULL, NULL};
ProtobufBroadcastPeer *peer_public_ = NULL;
ProtobufBroadcastPeer *peer_team_[4] = {NULL, NULL, NULL, NULL};
bool crypto_setup_ = false;

llsfrb::Configuration *config_;

std::string view2_time;
std::string view2_message;
std::string view2_log[10];
std::string view2_p[24];
std::string view2_oa[12];
std::string view2_ob[12];
std::string view2_otime[11];
std::string view2_odest[11];
std::string view2_t[10];
std::string view2_sigG[16];
std::string view2_sigY[16];
std::string view2_sigR[16];
std::string view2_pk[10];
std::string view2_robots;
std::string view2_state;
std::string view2_phase;
std::string view2_points;

std::string machine_name_;
std::string machine_type_;

llsfview2send sendData;
llsfview2recv recvData[3];

int m_type[11];
unsigned int gameHour, gameMin, gameSec;

std::string output_filename;

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


void init_message()
{
  int i;
  view2_time = "0";
  view2_message = "none";
  for (i = 0; i < 10; i++) view2_log[i] = ""; 
  for (i = 0; i <  2; i++) view2_p[i] = "S0";
  for (i = 0; i < 11; i++) {
    view2_oa[i] = "--";
    view2_ob[i] = "--";
    view2_otime[i] = "..:..-..:..";
    view2_odest[i] = "---";
  }
  for (i = 0; i < 10; i++) {view2_t[i] = "T1"; view2_pk[i] = "S0 S1 S2";}
  for (i = 0; i < 16; i++) {
    view2_sigG[i] = "sigG";
    view2_sigY[i] = "sigY";
    view2_sigR[i] = "sigR";
  }
  view2_robots = "none";
  view2_state = "none"; view2_phase = "none"; view2_points = "0";
}

void llsfInitOutputData()
{
  sendData.phase = 0;
  sendData.state = 0;
  for(int i = 0; i < 3; i++) {sendData.quantity_requested[i] = 0; sendData.quantity_delivered[i] = 0;};
  for(int i = 0; i < 5; i++) for(int j = 0; j < 3; j++) sendData.lspec_state[i][j] = 0;
  for(int i = 0; i < 10; i++) sendData.mType[i] = 0;
}

void llsfOutputView2(llsfview2send sendData)
{
  static int seqnumber;

  FILE *fp;
  if ((fp = fopen(sendData.filename.c_str(), "w")) != NULL) {
    fprintf(fp, "%d\t%d\t", sendData.phase, sendData.state);
    for(int i = 0; i <  3; i++) fprintf(fp, "%d\t", sendData.quantity_requested[i]);
    for(int i = 0; i <  3; i++) fprintf(fp, "%d\t", sendData.quantity_delivered[i]);
    for(int i = 0; i <  5; i++) for(int j = 0; j < 3; j++) fprintf(fp, "%d\t", sendData.lspec_state[i][j]);
    for(int i = 0; i < 10; i++) fprintf(fp, "%d\t", sendData.mType[i]);

    fprintf(fp, "%d", ++seqnumber);
    fclose(fp);
  }
}

void llsfInitInputFile(llsfview2recv &recvData)
{
  FILE *fp;
  recvData.machineNumber = 0;
  recvData.machineType = 0;
  recvData.seq = 0;

  if ((fp = fopen(recvData.filename.c_str(), "w")) != NULL) {
    fprintf(fp, "%d\t%d", 0, 0);
    fclose(fp);
  } else {
    printf("Cannot initialize the machine report file! %s\n", recvData.filename.c_str());
  }
}

void llsfInputView2(llsfview2recv &recvData)
{
  FILE *fp;
  if ((fp = fopen(recvData.filename.c_str(), "r")) != NULL) {
    while(feof(fp) == 0) {
      fscanf(fp, "%d\t%d\t%lf\t%lf\t%lf\t%ld", 
        &recvData.machineNumber, &recvData.machineType,
        &recvData.x, &recvData.y, &recvData.ori,
        &recvData.seq);
    } 
    fclose(fp);
  }
}

void outputInfo(){
  FILE *fp;
  int i;

  if((fp = fopen(output_filename.c_str(), "w")) == NULL) {
    printf("write error\n");
    return;
  }

  fprintf(fp, "TIME=\"%s\"\n", view2_time.c_str());
  fprintf(fp, "MESSAGE=\"%s\"\n", view2_message.c_str());
  for (i = 0; i < 10; i++) fprintf(fp, "LOG%d=\"%s\"\n", i + 1, view2_log[i].c_str());
  for (i = 0; i < 24; i++) fprintf(fp, "P%02d=\"%s\"\n", i + 1, view2_p[i].c_str());
  for (i = 0; i < 11; i++) fprintf(fp, "O%02da=\"%s\"; O%02db=\"%s\"; O%02dtime=\"%s\"; O%02ddest=\"%s\"\n", i + 1, view2_oa[i].c_str(), i + 1, view2_ob[i].c_str(), i + 1, view2_otime[i].c_str(), i + 1, view2_odest[i].c_str());
  for (i = 0; i < 10; i++) fprintf(fp, "T%02d=\"%s\"; PK%02d=\"%s\"\n", i + 1, view2_t[i].c_str(), i, view2_pk[i].c_str());
  for (i = 0; i < 16; i++) fprintf(fp, "SIG%02dG=\"%s\"; SIG%02dY=\"%s\"; SIG%02dR=\"%s\";\n", i + 1, view2_sigG[i].c_str(), i + 1, view2_sigY[i].c_str(), i + 1, view2_sigR[i].c_str());
  fprintf(fp, "ROBOTS=\"%s\"\n", view2_robots.c_str()); 
  fprintf(fp, "STATE=\"%s\"; PHASE=\"%s\"; POINTS=\"%s\";\n", view2_state.c_str(), view2_phase.c_str(), view2_points.c_str());
  fclose(fp);

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
    view2_robots = "Detected robot: " + b->team_name() + ":" + b->peer_name();

  }

  std::shared_ptr<GameState> gs;
  if ((gs = std::dynamic_pointer_cast<GameState>(msg))) {
    int hour = gs->game_time().sec() / 3600;
    int min  = (gs->game_time().sec() - hour * 3600) / 60;
    int sec  = gs->game_time().sec() - hour * 3600 - min * 60;

#if __WORDSIZE == 64
    printf("GameState received:  %02i:%02i:%02i.%02ld  %s %s  %u:%u points\n",
#else
    printf("GameState received:  %02i:%02i:%02i.%02lld  %s %s  %u:%u points\n",
#endif
           hour, min, sec, gs->game_time().nsec() / 1000000,
           llsf_msgs::GameState::Phase_Name(gs->phase()).c_str(),
           llsf_msgs::GameState::State_Name(gs->state()).c_str(),
           gs->points_cyan(), gs->points_magenta());
    char time[] = "00:00:00";
    sprintf(time, "%02i:%02i:%02i", hour, min, sec);
    view2_time = time;
    view2_phase = llsf_msgs::GameState::Phase_Name(gs->phase());
    view2_state = llsf_msgs::GameState::State_Name(gs->state());
    if (sendData.phase != gs->phase()) {
        init_message();
        llsfInitOutputData();
        for(int i = 0; i < 3; i++) llsfInitInputFile(recvData[i]);
    }
    char point[] = "0000:0000";
    sprintf(point, "%u:%u", gs->points_cyan(), gs->points_magenta());
    view2_points = point;
    // sendData[0] = (gs->phase() << 5) + ((gs->state()) << 3);
    sendData.phase = gs->phase();
    sendData.state = gs->state();

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
	  for(int i = 0; i < 4; i++) peer_team_[i]->setup_crypto(crypto_key, cipher);
	} catch (Exception &e) {
	  printf("No encryption key configured for team, not enabling crypto");
	}
      }
    } else if (crypto_setup_) {
      printf("Our team is not set, training game? Disabling crypto.\n");
      crypto_setup_ = false;
      for(int i = 0; i < 4; i++) peer_team_[i]->setup_crypto("", "");
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
      char time[] = "00:00-00:00", u[] = "0000";
      sprintf(time, "%02u:%02u-%02u:%02u", begin_min, begin_sec, end_min, end_sec);
      sprintf(u, "%u", o.quantity_delivered());
      view2_oa[o.id() - 1] = u;
      sprintf(u, "%u", o.quantity_requested());
      view2_ob[o.id() - 1] = u;
      view2_otime[o.id() - 1] = time;
      view2_odest[o.id() - 1] = llsf_msgs::Order::DeliveryGate_Name(o.delivery_gate());
      if ((begin_min * 60 + begin_sec <= gameMin * 60 + gameSec) && (gameMin * 60 + gameSec < end_min * 60 + end_sec)) { // Now on time!
        sendData.quantity_requested[o.product()] = o.quantity_requested();
        sendData.quantity_delivered[o.product()] = o.quantity_delivered();
      }
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
        sendData.lspec_state[es.type()[1] - '1'][lspec.color()] = lspec.state();
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
      switch (m.name()[0]) {
        case 'M': // Machine
          {
            sendData.mType[std::atoi(m.name().substr(1, 2).c_str())] = std::atoi(m.type().substr(1, 2).c_str());
          }
          break;
        case 'D': // Delivery Area
        case 'R': // Resycle Machine
        case 'T': // Test Machine
          break;
      }
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
static int playerNo = 0;
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

    playerNo = (playerNo + 1 ) % 3;
    llsfInputView2(recvData[playerNo]);

    if (seq_[playerNo] < recvData[playerNo].seq) {
      seq_[playerNo] = recvData[playerNo].seq;
      Pose2D *pose = signal->mutable_pose();
      pose->set_x(recvData[playerNo].x);
      pose->set_y(recvData[playerNo].y);
      pose->set_ori(recvData[playerNo].ori);

      Time *pose_time = pose->mutable_timestamp();
      pose_time->set_sec(4);
      pose_time->set_nsec(5);

      signal->set_number(playerNo + 1);
      signal->set_peer_name(name_[playerNo]);
      signal->set_team_name(team_name_);
      signal->set_team_color(team_);

      signal->set_seq(seq_[playerNo]);
      peer_public_->send(signal);
    }

    if (m_type[recvData[playerNo].machineNumber] != recvData[playerNo].machineType && recvData[playerNo].machineType > 0 && recvData[playerNo].machineNumber > 0) {
      machine_name_ = "M";
      machine_name_ += std::to_string(recvData[playerNo].machineNumber);
      machine_type_ = "T";
      machine_type_ += std::to_string(recvData[playerNo].machineType);

      printf("Announcing machine type (%s %s)\n", machine_name_.c_str(), machine_type_.c_str());
      llsf_msgs::MachineReport report;
      report.set_team_color(team_);
      llsf_msgs::MachineReportEntry *entry = report.add_machines();
      entry->set_name(machine_name_);
      entry->set_type(machine_type_);
      peer_public_->send(report);
    }
    timer_->expires_at(timer_->expires_at()
                      + boost::posix_time::milliseconds(500));
    timer_->async_wait(handle_timer);

  }

  llsfOutputView2(sendData);
  outputInfo();
}


int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "T:");

  if (argp.num_items() != 4) {
    printf("Usage: %s <name1> <name2> <name3> <team>\n", argv[0]);
    exit(1);
  }

  // sprintf(output_filename, "%s-var.sh", argv[0]);
  output_filename = argv[0];
  output_filename += "-var.sh";

  for(int i = 0; i < 3; i++) name_[i] = argp.items()[i];
  team_name_ = argp.items()[3];
  team_ = CYAN;
  if (argp.has_arg("T")) {
    std::string team_str = argp.arg("T");
    if (team_str == "cyan") {
      team_ = CYAN;
    } else if (team_str == "magenta") {
      team_ = MAGENTA;
    } else {
      printf("Unknonw team value, using cyan\n");
    }
  }
  init_message();

  llsfrb::Configuration *config = new llsfrb::YamlConfiguration(CONFDIR);
  config->load("config.yaml");

  if (config->exists("/llsfrb/comm/peer-send-port") &&
      config->exists("/llsfrb/comm/peer-recv-port") )
  {
      peer_public_ = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
                                           config_->get_uint("/llsfrb/comm/public-peer/recv-port"),
                                           config_->get_uint("/llsfrb/comm/public-peer/send-port"));
  } else {
    peer_public_ = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/public-peer/host"),
                                           config_->get_uint("/llsfrb/comm/public-peer/port"));
  }

  sendData.filename = config_->get_string("/llsfrb/comm/view2-send");

  boost::asio::io_service io_service;

  MessageRegister & message_register = peer_public_->message_register();
  message_register.add_message_type<BeaconSignal>();
  message_register.add_message_type<OrderInfo>();
  message_register.add_message_type<GameState>();
  message_register.add_message_type<VersionInfo>();
  message_register.add_message_type<ExplorationInfo>();
  message_register.add_message_type<MachineInfo>();
  message_register.add_message_type<MachineReport>();
  message_register.add_message_type<MachineReportInfo>();
  message_register.add_message_type<RobotInfo>();

  // sendData.filename = config->get_string("/llsfrb/comm/view2-send");
  // llsfInitOutputData();

  std::string cfg_prefix =
    std::string("/llsfrb/comm/") +
    ((team_color_ == CYAN) ? "cyan" : "magenta") + "-peer/";

  if (config_->exists((cfg_prefix + "send-port").c_str()) &&
      config_->exists((cfg_prefix + "recv-port").c_str()) )
  {
    for(int i = 1; i < 4; i++) {
      std::string robotino = "/llsfrb/comm/robotino";
      robotino += std::to_string(i);
      peer_team_[i] = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
					   config_->get_uint((cfg_prefix + "recv-port").c_str()),
					   config_->get_uint((cfg_prefix + "send-port").c_str()),
					   config_->get_string(robotino.c_str()),
					   &message_register /*, crypto_key, cipher*/);
    }
  } else {
    for(int i = 1; i < 4; i++) {
      std::string robotino = "/llsfrb/comm/robotino";
      robotino += std::to_string(i);
      peer_team_[i] = new ProtobufBroadcastPeer(config_->get_string((cfg_prefix + "host").c_str()),
					   config_->get_uint((cfg_prefix + "port").c_str()),
					   config_->get_string(robotino.c_str()),
					   &message_register/*, crypto_key, cipher*/);
    }
  }

  recvData[0].filename  = config_->get_string("/llsfrb/comm/view2-recv1");
  recvData[1].filename  = config_->get_string("/llsfrb/comm/view2-recv2");
  recvData[2].filename  = config_->get_string("/llsfrb/comm/view2-recv3");

  for(int i = 0; i < 3; i++) llsfInitInputFile(recvData[i]);

  for(int i = 0; i < 10; i++) m_type[i] = 0;
  machine_name_ = "M00";
  machine_type_ = "T0";

  peer_public_->signal_received().connect(handle_message);
  peer_public_->signal_recv_error().connect(handle_recv_error);
  peer_public_->signal_send_error().connect(handle_send_error);

  for(int i = 1; i < 4; i++) {
    peer_team_[i]->signal_received().connect(handle_message);
    peer_team_[i]->signal_recv_error().connect(handle_recv_error);
    peer_team_[i]->signal_send_error().connect(handle_send_error);
  }

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  timer_ = new boost::asio::deadline_timer(io_service);
  timer_->expires_from_now(boost::posix_time::milliseconds(500));
  timer_->async_wait(handle_timer);

  do {
    io_service.run();
    io_service.reset();
  } while (! quit);

  delete timer_;
  for(int i = 0; i < 4; i++) delete peer_team_[i];
  delete peer_public_;
  delete config_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
