
/***************************************************************************
 *  shell.h - LLSF RefBox shell
 *
 *  Created: Fri Feb 15 10:22:41 2013
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

#include "shell.h"
#include "machine.h"
#include "robot.h"
#include "order.h"
#include "menus.h"
#include "colors.h"

#include <protobuf_comm/client.h>
#include <config/yaml.h>

#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/MachineInfo.pb.h>
#include <msgs/MachineCommands.pb.h>
#include <msgs/AttentionMessage.pb.h>
#include <msgs/OrderInfo.pb.h>
#include <logging/llsf_log_msgs/LogMessage.pb.h>
#include <msgs/VersionInfo.pb.h>
#include <msgs/GameInfo.pb.h>
#include <msgs/RobotCommands.pb.h>

#include <cursesp.h>
#include <cursesf.h>
#include <cursesm.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <cstring>
#include <unistd.h>
#include <modbus/modbus.h>

//#include <libs/llsf_sps/mps_band.h>

// defined in miliseconds
#define TIMER_INTERVAL 500
#define RECONNECT_TIMER_INTERVAL 1000
#define BLINK_TIMER_INTERVAL 250
#define ATTMSG_TIMER_INTERVAL 1000
#define MIN_NUM_ROBOTS 6

using namespace protobuf_comm;

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


LLSFRefBoxShell::LLSFRefBoxShell()
  : quit_(false), error_(NULL), panel_(nullptr), navbar_(nullptr),
    rb_log_(nullptr), p_orders_(nullptr), p_attmsg_(nullptr), p_state_(nullptr),
    p_phase_(nullptr), p_time_(nullptr), p_points_(nullptr), p_team_cyan_(nullptr),
    p_team_magenta_(nullptr), m_state_(nullptr),  m_phase_(nullptr),
    timer_(io_service_), reconnect_timer_(io_service_), try_reconnect_(true),
    blink_timer_(io_service_), attmsg_timer_(io_service_), attmsg_toggle_(true),
    attmsg_team_specific_(false), beep_warning_shown_(false)
{
  stdin_ = new boost::asio::posix::stream_descriptor(io_service_, dup(STDIN_FILENO));
  client = new ProtobufStreamClient();
  config_ = new llsfrb::YamlConfiguration(CONFDIR);
  config_->load("config.yaml");
}


LLSFRefBoxShell::~LLSFRefBoxShell()
{
  quit_ = true;
  io_service_.stop();
  try_reconnect_ = false;
      
  timer_.cancel();
  reconnect_timer_.cancel();
  blink_timer_.cancel();
  attmsg_timer_.cancel();
  stdin_->cancel();
#if BOOST_ASIO_VERSION > 100409
  stdin_->release();
#endif
  try {
    delete stdin_;
    stdin_ = nullptr;
  } catch (std::exception &e) {} // ignored

  last_minfo_.reset();
  last_gameinfo_.reset();
  last_robotinfo_.reset();

  delete client;
  client = 0;
  delete panel_;
  delete navbar_;

  std::map<std::string, LLSFRefBoxShellMachine *>::iterator m;
  for (m = machines_.begin(); m != machines_.end(); ++m) {
    delete m->second;
  }
  machines_.clear();

  for (size_t i = 0; i < robots_.size(); ++i) {
    delete robots_[i];
  }
  robots_.clear();

  for (size_t i = 0; i < orders_.size(); ++i) {
    delete orders_[i];
  }
  orders_.clear();

  delete rb_log_;

  delete m_state_;
  delete m_phase_;

  delete p_state_;
  delete p_phase_;
  delete p_time_;
  delete p_points_;
  delete p_team_cyan_;
  delete p_team_magenta_;
  delete p_attmsg_;

  delete config_;
}


const char *
LLSFRefBoxShell::error() const
{
  return error_;
}


/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBoxShell::handle_signal(const boost::system::error_code& error, int signum)
{
  timer_.cancel();
  reconnect_timer_.cancel();
  blink_timer_.cancel();
  attmsg_timer_.cancel();
  io_service_.stop();
}



/** Start the timer for another run. */
void
LLSFRefBoxShell::start_timers()
{
  timer_.expires_from_now(boost::posix_time::milliseconds(TIMER_INTERVAL));
  timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_timer, this,
				boost::asio::placeholders::error));
  blink_timer_.expires_from_now(boost::posix_time::milliseconds(BLINK_TIMER_INTERVAL));
  blink_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_blink_timer, this,
				      boost::asio::placeholders::error));
}

/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBoxShell::handle_timer(const boost::system::error_code& error)
{
  if (! error) {
    for (size_t i = 0; i < robots_.size(); ++i) {
      robots_[i]->refresh();
    }

    timer_.expires_at(timer_.expires_at()
		      + boost::posix_time::milliseconds(TIMER_INTERVAL));
    timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_timer, this,
				  boost::asio::placeholders::error));
  }
}

/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBoxShell::handle_blink_timer(const boost::system::error_code& error)
{
  if (! error) {

    std::map<std::string, LLSFRefBoxShellMachine *>::iterator m;
    for (m = machines_.begin(); m != machines_.end(); ++m) {
      m->second->flip_blink_states();
    }
    blink_timer_.expires_at(blink_timer_.expires_at()
			    + boost::posix_time::milliseconds(BLINK_TIMER_INTERVAL));
    blink_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_blink_timer, this,
					boost::asio::placeholders::error));
  }
}

void
LLSFRefBoxShell::start_keyboard()
{
  stdin_->async_read_some(boost::asio::null_buffers(),
			  boost::bind(&LLSFRefBoxShell::handle_keyboard, this,
				      boost::asio::placeholders::error));
}


/** Handle keyboard input.
 * @param error error code
 */
void
LLSFRefBoxShell::handle_keyboard(const boost::system::error_code& error)
{
  if (! error) {
    int c = panel_->getch();
    if (c != ERR) {
      switch (c) {
      case ' ':
	if (s_state_ == "WAIT_START" || s_state_ == "PAUSED") {
	  set_game_state("RUNNING");
	} else {
	  set_game_state("PAUSED");
	}
	break;
      case 'Q':
      case 'q':
	io_service_.stop();
	break;
      case KEY_F(2):
	(*m_state_)();
	io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
	break;
      case KEY_F(3):
	(*m_phase_)();
	io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
	break;

      case KEY_F(4):
	if (last_gameinfo_) {
	  TeamColorSelectMenu tcsm(panel_);
	  tcsm();
	  if (tcsm) {
	    try {
	      TeamSelectMenu m(panel_, tcsm.get_team_color(), last_gameinfo_, last_game_state_);
	      m();
	      if (m) {
		std::string team_name = m.get_team_name();
		send_set_team(tcsm.get_team_color(), team_name);	      
	      }
	    } catch (NCursesException &e) {
	      logf("Machine menu failed: %s", e.message);
	    }
	  }
	  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
	}
	break;

      case KEY_F(9):
	if (last_robotinfo_) {
	  TeamColorSelectMenu tcsm(panel_);
	  tcsm();
	  if (tcsm) {
	    try {
	      RobotMaintenanceMenu rmm(panel_, tcsm.get_team_color(), last_robotinfo_);
	      rmm();
	      if (rmm) {
		unsigned int robot_number;
		bool maintenance;
		rmm.get_robot(robot_number, maintenance);
		send_robot_maintenance(tcsm.get_team_color(), robot_number, maintenance);
	      }
	    } catch (NCursesException &e) {
	      logf("Machine menu failed: %s", e.message);
	    }
	  }
	  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
	} else {
	  logf("No robot info received, yet.");
	}
	break;

      case KEY_F(12):
	if (! last_orderinfo_) {
	  logf("No order info received, yet.");
	} else if (! last_game_state_) {
	  logf("No game state received, yet.");
	} else {
	  TeamColorSelectMenu tcsm(panel_);
	  tcsm();
		if (tcsm) {
			OrderDeliverMenu odm(
				panel_, tcsm.get_team_color(), last_orderinfo_, last_game_state_);
			odm();
			if (odm) {
				if (odm.show_all()) {
					SelectOrderByIDMenu oidm(panel_,
								                   tcsm.get_team_color(),
								                   last_orderinfo_,
								                   last_game_state_);
					oidm();
					if (oidm) { send_delivery_by_order_id(oidm.order().id(), tcsm.get_team_color()); }
				} else {
					DeliveryCorrectMenu dcm(panel_, tcsm.get_team_color(), odm.delivery(), last_orderinfo_);
					dcm();
					if (dcm) { send_confirm_delivery(odm.delivery(), dcm.correct()); }
				}
			}
		}
	  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
	}
	break;
      }
    }
    start_keyboard();
  }
}


/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBoxShell::handle_attmsg_timer(const boost::system::error_code& error)
{
  std::lock_guard<std::mutex> lock(attmsg_mutex_);
  if (! error) {
    if (attmsg_has_endtime_) {
      boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
      boost::posix_time::time_duration td = now - attmsg_endtime_;
      if (td > boost::posix_time::seconds(0)) {
	attmsg_string_ = "";
	attmsg_has_endtime_ = false;
	p_attmsg_->bkgd(' '|COLOR_PAIR(0));
	p_attmsg_->erase();
      }
    }

    if (attmsg_string_ != "") {
      p_attmsg_->erase();
      p_attmsg_->standend();
      attmsg_toggle_ = ! attmsg_toggle_;

      if (attmsg_toggle_) {
	if (attmsg_team_specific_) {
	  if (attmsg_team_ == llsf_msgs::CYAN) {
	    p_attmsg_->bkgd(' '|COLOR_PAIR(COLOR_CYAN_ON_BACK));
	    p_attmsg_->attron(' '|COLOR_PAIR(COLOR_CYAN_ON_BACK)|A_BOLD);
	  } else {
	    p_attmsg_->bkgd(' '|COLOR_PAIR(COLOR_MAGENTA_ON_BACK));
	    p_attmsg_->attron(' '|COLOR_PAIR(COLOR_MAGENTA_ON_BACK)|A_BOLD);
	  }
	} else {
	  p_attmsg_->bkgd(' '|COLOR_PAIR(COLOR_RED_ON_BACK));
	  p_attmsg_->attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK)|A_BOLD);
	}
      } else {
	beep(1500, ATTMSG_TIMER_INTERVAL);
	if (attmsg_team_specific_) {
	  if (attmsg_team_ == llsf_msgs::CYAN) {
	    p_attmsg_->bkgd(' '|COLOR_PAIR(COLOR_WHITE_ON_CYAN));
	    p_attmsg_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_CYAN)|A_BOLD);
	  } else {
	    p_attmsg_->bkgd(' '|COLOR_PAIR(COLOR_WHITE_ON_MAGENTA));
	    p_attmsg_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_MAGENTA)|A_BOLD);
	  }
	} else {
	  p_attmsg_->bkgd(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
	  p_attmsg_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED)|A_BOLD);
	}
      }
      if ((int)attmsg_string_.length() >= p_attmsg_->width()) {
	p_attmsg_->addstr(attmsg_string_.c_str());
      } else {
	p_attmsg_->addstr(0, (p_attmsg_->width() - attmsg_string_.length()) / 2,
			  attmsg_string_.c_str());
      }
      p_attmsg_->standend();

      attmsg_timer_.expires_at(attmsg_timer_.expires_at()
			       + boost::posix_time::milliseconds(ATTMSG_TIMER_INTERVAL));
      attmsg_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_attmsg_timer, this,
					   boost::asio::placeholders::error));      
    } else {
      p_attmsg_->erase();
    }

    io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
  }
}


/** Handle reconnect timer event.
 * @param error error code
 */
void
LLSFRefBoxShell::handle_reconnect_timer(const boost::system::error_code& error)
{
  if (! error && try_reconnect_ && ! quit_) {
    client->async_connect(cfg_refbox_host_.c_str(), cfg_refbox_port_);
  }
}


void
LLSFRefBoxShell::set_game_state(std::string state)
{
  llsf_msgs::GameState::State value;
  if (llsf_msgs::GameState_State_Parse(state, &value)) {
    logf("Requesting new game state %s", state.c_str());
    llsf_msgs::SetGameState msg;
    msg.set_state(value);
    try {
      client->send(msg);
    } catch (std::runtime_error &e) {
      logf("Sending game state failed: %s", e.what());
    }
  } else {
    logf("Failed to parse game state %s", state.c_str());
  }
}


void
LLSFRefBoxShell::set_game_phase(std::string phase)
{
  llsf_msgs::GameState::Phase value;
  if (llsf_msgs::GameState_Phase_Parse(phase, &value)) {
    logf("Requesting new game phase %s", phase.c_str());
    llsf_msgs::SetGamePhase msg;
    msg.set_phase(value);
    try {
      client->send(msg);
    } catch (std::runtime_error &e) {
      logf("Sending game phase failed: %s", e.what());
    }
  } else {
    logf("Failed to parse game phase %s", phase.c_str());
  }
}


void
LLSFRefBoxShell::send_set_team(llsf_msgs::Team team, std::string &team_name)
{
  llsf_msgs::SetTeamName msg;
  msg.set_team_name(team_name);
  msg.set_team_color(team);
  try {
    client->send(msg);
  } catch (std::runtime_error &e) {
    logf("Setting team info failed: %s", e.what());
  }
}

void
LLSFRefBoxShell::send_robot_maintenance(llsf_msgs::Team team,
					unsigned int robot_number, bool maintenance)
{
  llsf_msgs::SetRobotMaintenance msg;
  msg.set_robot_number(robot_number);
  msg.set_team_color(team);
  msg.set_maintenance(maintenance);
  logf("%sabling maintenance of robot %u", maintenance ? "En" : "Dis", robot_number);
  try {
    client->send(msg);
  } catch (std::runtime_error &e) {
    logf("Sending SetRobotMaintenance failed: %s", e.what());
  }
}

void
LLSFRefBoxShell::send_confirm_delivery(unsigned int delivery_id, bool correct)
{
	llsf_msgs::ConfirmDelivery od;
	od.set_delivery_id(delivery_id);
	od.set_correct(correct);
	if (correct) {
		logf("Confirming correct delivery %u", delivery_id);
	} else {
		logf("Confirming incorrect delivery %u", delivery_id);
	}
	try {
		client->send(od);
	} catch (std::runtime_error &e) {
		logf("Sending ConfirmDelivery failed: %s", e.what());
	}
}

void
LLSFRefBoxShell::send_delivery_by_order_id(unsigned int order_id, llsf_msgs::Team team)
{
  llsf_msgs::SetOrderDelivered od;
  od.set_order_id(order_id);
  od.set_team_color(team);
  try {
    client->send(od);
  } catch(std::runtime_error &e) {
    logf("Sending SetOrderDelivered failed: %s", e.what());
  }
}

void
LLSFRefBoxShell::client_connected()
{
  p_state_->erase();
  p_state_->standend();
  p_state_->addstr("CONNECTED");
  p_phase_->erase();
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
}

void
LLSFRefBoxShell::dispatch_client_connected()
{
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::client_connected, this));
}


void
LLSFRefBoxShell::refresh()
{
  panel_->refresh();
}

void
LLSFRefBoxShell::client_disconnected(const boost::system::error_code &error)
{
  p_state_->erase();
  p_phase_->erase();
  p_time_->erase();
  p_points_->erase();
  p_team_cyan_->erase();
  p_team_magenta_->erase();
  p_state_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED)|A_BOLD);
  p_state_->addstr("DISCONNECTED");
  p_state_->standend();
  //p_state_->addstr(error.message().c_str());

  if (! quit_) {
    std::lock_guard<std::mutex> lock(attmsg_mutex_);
    attmsg_string_ = "";
    p_attmsg_->bkgd(' '|COLOR_PAIR(0));
    p_attmsg_->erase();

    // Erase version string
    panel_->hline(panel_->height() - 1, 2, 20);

    // Erase refbox mode string
    panel_->hline(panel_->height() - 1, panel_->width() - 35, 8);

    std::map<std::string, LLSFRefBoxShellMachine *>::iterator m;
    for (m = machines_.begin(); m != machines_.end(); ++m) {
      m->second->reset();
    }

    for (size_t i = 0; i < orders_.size(); ++i) {
      orders_[i]->reset();
    }

    for (size_t i = 0; i < robots_.size(); ++i) {
      robots_[i]->reset();
    }

    if (try_reconnect_) {
      reconnect_timer_.expires_from_now(boost::posix_time::milliseconds(RECONNECT_TIMER_INTERVAL));
      reconnect_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_reconnect_timer, this,
					      boost::asio::placeholders::error));
    }

  }
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
}

void
LLSFRefBoxShell::dispatch_client_disconnected(const boost::system::error_code &error)
{
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::client_disconnected, this, error));
}

void
LLSFRefBoxShell::dispatch_client_msg(uint16_t comp_id, uint16_t msg_type,
				     std::shared_ptr<google::protobuf::Message> msg)
{
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::client_msg, this, comp_id, msg_type, msg));
}

void
LLSFRefBoxShell::client_msg(uint16_t comp_id, uint16_t msg_type,
			    std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<llsf_msgs::GameState> g;
  if ((g = std::dynamic_pointer_cast<llsf_msgs::GameState>(msg))) {
    last_game_state_ = g;
    s_state_ = llsf_msgs::GameState::State_Name(g->state());
    p_state_->erase();
    if (g->state() == llsf_msgs::GameState::WAIT_START ||
	g->state() == llsf_msgs::GameState::PAUSED)
    {
      p_state_->attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK)|A_BOLD);
    } else if (g->state() == llsf_msgs::GameState::RUNNING) {
      p_state_->attron(' '|COLOR_PAIR(COLOR_GREEN_ON_BACK)|A_BOLD);
    }
    p_state_->addstr(llsf_msgs::GameState::State_Name(g->state()).c_str());
    p_state_->standend();

    p_phase_->erase();
    p_phase_->addstr(llsf_msgs::GameState::Phase_Name(g->phase()).c_str());

    int hour = g->game_time().sec() / 3600;
    int min  = (g->game_time().sec() - hour * 3600) / 60;
    int sec  = g->game_time().sec() - hour * 3600 - min * 60;
    p_time_->erase();
    if (hour > 0) {
#if __WORDSIZE == 64
      p_time_->printw("%02d:%02d:%02d.%03ld", hour, min, sec, g->game_time().nsec() / 1000000);
#else
      p_time_->printw("%02d:%02d:%02d.%03lld", hour, min, sec, g->game_time().nsec() / 1000000);
#endif
    } else {
#if __WORDSIZE == 64
      p_time_->printw("%02d:%02d.%03ld", min, sec, g->game_time().nsec() / 1000000);
#else
      p_time_->printw("%02d:%02d.%03lld", min, sec, g->game_time().nsec() / 1000000);
#endif
    }

    for (size_t i = 0; i < orders_.size(); ++i) {
      orders_[i]->set_gametime(g->game_time().sec());
    }

    p_points_->erase();
    if (g->points_cyan() > g->points_magenta()) {
      p_points_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_CYAN)|A_BOLD);
    } else {
      p_points_->attron(' '|COLOR_PAIR(COLOR_CYAN_ON_BACK));
    }
    if (g->has_points_cyan()) {
      p_points_->printw("%3u", g->points_cyan());
    } else {
      p_points_->printw("---");
    }
    p_points_->standend();
    p_points_->printw(" / ");
    if (g->points_magenta() > g->points_cyan()) {
      p_points_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_MAGENTA)|A_BOLD);
    } else {
      p_points_->attron(' '|COLOR_PAIR(COLOR_MAGENTA_ON_BACK)|A_BOLD);
    }
    if (g->has_points_magenta()) {
      p_points_->printw("%3u", g->points_magenta());
    } else {
      p_points_->printw("---");
    }
    p_points_->standend();

    p_team_cyan_->erase();
    p_team_magenta_->erase();
    if (g->has_team_cyan()) {
      p_team_cyan_->printw("%s", g->team_cyan().c_str());
    }
    if (g->has_team_magenta()) {
      p_team_magenta_->printw("%s", g->team_magenta().c_str());
    }

    if (! g->has_team_cyan() && ! g->has_team_magenta() &&
	g->phase() != llsf_msgs::GameState::PRE_GAME)
    {
      p_team_cyan_->attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK));
      p_team_cyan_->printw("** TRAINING **");
      p_team_cyan_->standend();
    }
    navbar_->standend();

    if (g->state() == llsf_msgs::GameState::WAIT_START) {
      navbar_->attron(' '|COLOR_PAIR(COLOR_GREEN_ON_BACK)|A_BOLD);
      navbar_->addstr(0, navbar_->cols() - 5, "STRT");
    } else if (g->state() == llsf_msgs::GameState::PAUSED) {
      navbar_->attron(' '|COLOR_PAIR(COLOR_GREEN_ON_BACK)|A_BOLD);
      navbar_->addstr(0, navbar_->cols() - 5, "CONT");
    } else {
      navbar_->attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK)|A_BOLD);
      navbar_->addstr(0, navbar_->cols() - 5, "STOP");
    }

    /*
    // RefBox mode
    if (g->refbox_mode() != llsf_msgs::GameState::STANDALONE) {
      panel_->standend();
      panel_->attron(A_BOLD);
      panel_->printw(panel_->height() - 1, panel_->width() - 35,
		     " %s ", llsf_msgs::GameState_RefBoxMode_Name(g->refbox_mode()).c_str());
      panel_->standend();
    }
    */


  }

  std::shared_ptr<llsf_msgs::GameInfo> ginfo;
  if ((ginfo = std::dynamic_pointer_cast<llsf_msgs::GameInfo>(msg))) {
    last_gameinfo_ = ginfo;
  }

  std::shared_ptr<llsf_msgs::RobotInfo> r;
  if ((r = std::dynamic_pointer_cast<llsf_msgs::RobotInfo>(msg))) {
    last_robotinfo_ = r;
    size_t idx = 0;
    for (int i = 0; i < r->robots_size(); ++i, ++idx) {
      const llsf_msgs::Robot &robot = r->robots(i);

      if ((size_t)idx >= robots_.size()) {
	// more robots than we can show
	logf("Max robots displayed, cannot show %s (%s)",
	     robot.name().c_str(), robot.team().c_str());
	break;
      }

      robots_[idx]->update(robot.number(), robot.name(), robot.team(), robot.team_color(),
			   robot.host(), robot.state(), robot.maintenance_time_remaining(),
			   robot.maintenance_cycles());
      boost::posix_time::ptime
	last_seen(boost::posix_time::from_time_t(robot.last_seen().sec()));
      last_seen += boost::posix_time::nanoseconds(robot.last_seen().nsec());
      robots_[idx]->set_last_seen(last_seen);
    }
    for (size_t i = idx+1; i < robots_.size(); ++i) {
      robots_[i]->reset();
    }
  }

  std::shared_ptr<llsf_msgs::MachineInfo> minfo;
  if ((minfo = std::dynamic_pointer_cast<llsf_msgs::MachineInfo>(msg))) {
    last_minfo_ = minfo;
    for (int i = 0; i < minfo->machines_size(); ++i) {
      std::map<std::string, LLSFRefBoxShellMachine *>::iterator mpanel;
      const llsf_msgs::Machine &mspec = minfo->machines(i);
      //logf("Adding %s @ (%f, %f, %f)\n", mspec.name().c_str(),
      //     mspec.pose().x(), mspec.pose().y(), mspec.pose().ori());
      if ((mpanel = machines_.find(mspec.name())) != machines_.end()) {
	mpanel->second->update(mspec);
	mpanel->second->refresh();
      }
    }
  }

  std::shared_ptr<llsf_msgs::AttentionMessage> am;
  if ((am = std::dynamic_pointer_cast<llsf_msgs::AttentionMessage>(msg))) {
    std::lock_guard<std::mutex> lock(attmsg_mutex_);

    if (attmsg_string_ != "") {
      attmsg_timer_.cancel();
    }
    attmsg_string_ = am->message();
    attmsg_team_specific_ = am->has_team_color();
    attmsg_has_endtime_ = am->has_time_to_show();
    if (attmsg_team_specific_) {
      attmsg_team_ = am->team_color();
    }
    if (attmsg_has_endtime_) {
      boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
      attmsg_endtime_ = now + boost::posix_time::seconds(am->time_to_show());
    }
    if (attmsg_string_ != "") {
      log(llsf_log_msgs::LogMessage::LL_ERROR, "A", "%s", attmsg_string_.c_str());

      attmsg_toggle_ = true;
      attmsg_timer_.expires_from_now(boost::posix_time::milliseconds(0));
      attmsg_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_attmsg_timer, this,
					   boost::asio::placeholders::error));
    }
  }

  std::shared_ptr<llsf_msgs::OrderInfo> ordins;
  if ((ordins = std::dynamic_pointer_cast<llsf_msgs::OrderInfo>(msg))) {
    last_orderinfo_ = ordins;
    const size_t size = std::min((size_t)ordins->orders_size(), (size_t)orders_.size());
    size_t oidx = 0;
    for (size_t i = 0; i < size; ++i) {
      const llsf_msgs::Order &ospec = ordins->orders(i);
      std::vector<llsf_msgs::RingColor> ring_colors(ospec.ring_colors_size());
      for (int j = 0; j < ospec.ring_colors_size(); ++j) {
        ring_colors[j] = ospec.ring_colors(j);
      }
      orders_[oidx++]->update(ospec.id(), ospec.complexity(),
        ospec.competitive(),
        ospec.base_color(), ring_colors, ospec.cap_color(),
        ospec.quantity_requested(),
        ospec.quantity_delivered_cyan(),
        ospec.quantity_delivered_magenta(),
        ospec.delivery_period_begin(),
        ospec.delivery_period_end(), ospec.delivery_gate());
    }
    for (size_t i = oidx; i < orders_.size(); ++i) {
      orders_[i]->reset();
    }
  }

  std::shared_ptr<llsf_log_msgs::LogMessage> lm;
  if ((lm = std::dynamic_pointer_cast<llsf_log_msgs::LogMessage>(msg))) {
    log(lm->log_level(), lm->ts_sec(), lm->ts_nsec(), lm->component(), lm->message());
  }

  std::shared_ptr<llsf_msgs::VersionInfo> vi;
  if ((vi = std::dynamic_pointer_cast<llsf_msgs::VersionInfo>(msg))) {
    logf("Connected to RefBox version %s", vi->version_string().c_str());
    panel_->standend();
    panel_->attron(A_BOLD);
    panel_->printw(panel_->height()-1, 2, //panel_->width() - vi->version_string().length() - 10,
		   " RefBox %s ", vi->version_string().c_str());
    panel_->standend();
  }

  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
}

void
LLSFRefBoxShell::log(llsf_log_msgs::LogMessage::LogLevel log_level,
		     long int ts_sec, long int ts_nsec,
		     const std::string &component, const std::string &message)
{
  rb_log_->standend();
  rb_log_->attron(' '|COLOR_PAIR(COLOR_DEFAULT));
  if (log_level == llsf_log_msgs::LogMessage::LL_DEBUG) {
    rb_log_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BACK));
  } else if (log_level == llsf_log_msgs::LogMessage::LL_INFO) {
    rb_log_->attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
  } else if (log_level == llsf_log_msgs::LogMessage::LL_WARN) {
    rb_log_->attron(' '|COLOR_PAIR(COLOR_YELLOW_ON_BACK));
  } else if (log_level == llsf_log_msgs::LogMessage::LL_ERROR) {
    rb_log_->attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK));
  }
  struct ::tm now_s;
  if (ts_sec == 0) {
    struct timeval now;
    gettimeofday(&now, NULL);
    ts_sec = now.tv_sec;
    ts_nsec = now.tv_usec * 1000;
  }
  time_t t = ts_sec;
  localtime_r(&t, &now_s);
  rb_log_->printw("%02d:%02d:%02d.%03ld %s: %s\n", now_s.tm_hour,
		  now_s.tm_min, now_s.tm_sec, ts_nsec / 1000000,
		  component.c_str(), message.c_str());
  rb_log_->standend();
}

void
LLSFRefBoxShell::log(llsf_log_msgs::LogMessage::LogLevel log_level,
		     const std::string &component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  char *tmp;
  if (vasprintf(&tmp, format, arg) != -1) {
    log(log_level, 0, 0, component, tmp);
    free(tmp);
  }
  va_end(arg);
}


void
LLSFRefBoxShell::logf(const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  char *tmp;
  if (vasprintf(&tmp, format, arg) != -1) {
    log(llsf_log_msgs::LogMessage::LL_INFO, 0, 0, "L", tmp);
    free(tmp);
  }
  va_end(arg);
}

void
LLSFRefBoxShell::beep(int frequency, int duration_ms)
{
  if (access("/sys/module/pcspkr/initstate", R_OK) == -1) {
	  if (! beep_warning_shown_) {
	    //log(llsf_log_msgs::LogMessage::LL_WARN, "L", "%s",
	    //  "Cannot beep. Kernel module pcspkr seems not to be loaded");
      beep_warning_shown_ = true;
    }
    return;
  }

  FILE *tty;
  if ( NULL == (tty = fopen ("/dev/tty0", "w")) ) {
    if (! beep_warning_shown_) {
      log(llsf_log_msgs::LogMessage::LL_WARN, "L", "%s",
	  "Cannot beep. No write access to /dev/tty0. Is user in tty group?");
      beep_warning_shown_ = true;
    }
    return;
  }
  char ESC = 27;
  fprintf(tty, "%c[10;%d]%c[11;%d]\a", ESC, frequency, ESC, duration_ms);
  fclose(tty);
}

int
LLSFRefBoxShell::run()
{
  cfg_refbox_host_ = config_->get_string("/llsfrb/shell/refbox-host");
  cfg_refbox_port_ = config_->get_uint("/llsfrb/shell/refbox-port");

  panel_ = new NCursesPanel(LINES - 1, COLS);
  navbar_ = new NCursesPanel(1, COLS, LINES - 1, 0);

  curs_set(0); // invisible cursor
  use_default_colors();
  init_colors();

  int height = panel_->maxy();

  panel_->bkgd(' '|COLOR_PAIR(0));
  panel_->frame();
  panel_->vline(1,                  panel_->width() - 30, height);
  panel_->addch(0,                  panel_->width() - 30, ACS_TTEE);
  panel_->addch(height, panel_->width() - 30, ACS_BTEE);

  panel_->hline(height - 7, panel_->width() - 29, 28);
  panel_->addch(height - 7, panel_->width() - 30, ACS_LTEE);
  panel_->addch(height - 7, panel_->width() -  1, ACS_RTEE);

  panel_->hline(15, panel_->width() - 29, 28);
  panel_->addch(15, panel_->width() - 30, ACS_LTEE);
  panel_->addch(15, panel_->width() -  1, ACS_RTEE);

  panel_->hline(2, 1, panel_->width() - 30);
  panel_->addch(2, 0, ACS_LTEE);
  panel_->addch(2, panel_->width() - 30, ACS_RTEE);

  int rb_log_lines   = height - 10;

  //panel_->hline(rb_log_lines + 3, 1, panel_->width() - 26);
  //panel_->addch(rb_log_lines + 3, 0, ACS_LTEE);
  //panel_->addch(rb_log_lines + 3, panel_->width() - 26, ACS_RTEE);

  panel_->hline(height - 7, 1, panel_->width() - 30);
  panel_->addch(height - 7, 0, ACS_LTEE);
  panel_->addch(height - 7, panel_->width() - 30, ACS_PLUS);

  panel_->attron(A_BOLD);
  panel_->addstr(0, panel_->width() - 17, "Machines");
  panel_->addstr(0, (panel_->width() - 26) / 2 - 7, "Attention Message");
  panel_->addstr(2, (panel_->width() - 26) / 2 - 4, "RefBox Log");
  panel_->addstr(15, panel_->width() - 16, "Robots");
  panel_->addstr(height-7, panel_->width() - 15, "Game");
  panel_->addstr(height-7, (panel_->width() - 26) / 2 - 2, "Orders");
  panel_->attroff(A_BOLD);

  panel_->attron(A_BOLD);
  panel_->addstr(height-6, panel_->width() - 28, "State:");
  panel_->addstr(height-5, panel_->width() - 28, "Phase:");
  panel_->addstr(height-4, panel_->width() - 28, "Time:");
  panel_->addstr(height-3, panel_->width() - 28, "Points:");
  panel_->addstr(height-2, panel_->width() - 28, "Cyan:");
  panel_->addstr(height-1, panel_->width() - 28, "Magenta:");
  panel_->attroff(A_BOLD);

  panel_->show();
  panel_->refresh();

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 1, "F2");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 4, "STATE");

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 11, "F3");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 14, "PHASE");

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 21, "F4");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 24, "TEAM");

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 30, "F9");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 33, "ROBOT");

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 40, "F12");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 44, "DELIVER");

  navbar_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED)|A_BOLD);
  navbar_->addstr(0, navbar_->cols() - 9, "SPC");
  navbar_->standend();
  navbar_->attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK)|A_BOLD);
  navbar_->addstr(0, navbar_->cols() - 5, "STOP");
  navbar_->refresh();

  rb_log_ = new NCursesPanel(rb_log_lines,  panel_->width() - 34, 3, 1);
  rb_log_->scrollok(TRUE);

  const int mx = panel_->width() - 29;
  std::vector<std::string> mnames = {"C-BS", "C-DS", "C-SS", "C-RS1", "C-RS2", "C-CS1", "C-CS2",
				     "M-BS", "M-DS", "M-SS", "M-RS1", "M-RS2", "M-CS1", "M-CS2"};
  for (size_t m = 0; m < mnames.size(); ++m) {
    std::string &mname = mnames[m];
    std::string type = mname.substr(2, 2);
    machines_[mname] =
      new LLSFRefBoxShellMachine(mname,  type,  m+1, mx);
  }

  std::map<std::string, LLSFRefBoxShellMachine *>::iterator m;
  for (m = machines_.begin(); m != machines_.end(); ++m) {
    m->second->refresh();
  }

  const int robots_max_height = (panel_->height() - 24);
  const int max_robots = std::max(MIN_NUM_ROBOTS, robots_max_height/2);

  robots_.resize(max_robots, NULL);
  for (size_t i = 0; i < robots_.size(); ++i) {
    robots_[i] = new LLSFRefBoxShellRobot(16 + 2 * i, panel_->width() - 29);
    robots_[i]->refresh();
  }

  orders_.resize(12, NULL);
  for (size_t i = 0; i < orders_.size(); ++i) {
    orders_[i] = new LLSFRefBoxShellOrder(height - 6 + i / 2,
					  (i % 2) ? ((panel_->width() - 27) / 2) : 1);
    orders_[i]->refresh();
  }

  panel_->refresh();

  p_attmsg_       = new NCursesPanel(1, panel_->width() - 31, 1, 1);
  p_state_        = new NCursesPanel(1, 14, height-6,  panel_->width() - 19);
  p_phase_        = new NCursesPanel(1, 14, height-5,  panel_->width() - 19);
  p_time_         = new NCursesPanel(1, 14, height-4,  panel_->width() - 19);
  p_points_       = new NCursesPanel(1, 14, height-3,  panel_->width() - 19);
  p_team_cyan_    = new NCursesPanel(1, 14, height-2,  panel_->width() - 19);
  p_team_magenta_ = new NCursesPanel(1, 14, height-1,  panel_->width() - 19);

  p_state_->attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED)|A_BOLD);
  p_state_->addstr("DISCONNECTED");
  p_state_->standend();

  p_team_cyan_->addstr("");
  p_team_magenta_->addstr("");

  // State menu
  const google::protobuf::EnumDescriptor *state_enum_desc =
    llsf_msgs::GameState_State_descriptor();
  const int num_state_values = state_enum_desc->value_count() - 1;
  NCursesMenuItem **state_items = new NCursesMenuItem*[2 + num_state_values];
  int item_i = 0;
  for (int i = 0; i < state_enum_desc->value_count(); ++i) {
    if (state_enum_desc->value(i)->name() == "INIT")  continue;
    SignalItem *item = new SignalItem(state_enum_desc->value(i)->name());
    item->signal().connect(boost::bind(&LLSFRefBoxShell::set_game_state, this,
				       state_enum_desc->value(i)->name()));
    state_items[item_i++] = item;
  }
  s_cancel_state_ = "** CANCEL **";
  state_items[num_state_values]   = new SignalItem(s_cancel_state_);
  // termination sentinel element
  state_items[num_state_values+1] = new NCursesMenuItem();

  try {
    m_state_ = new GenericItemsMenu(panel_, num_state_values+1, state_items);
    m_state_->frame("Set State");
    m_state_->hide();
  } catch (NCursesException &e) {
    logf("%s", e.message);
  }

  // Phase menu
  const google::protobuf::EnumDescriptor *phase_enum_desc =
    llsf_msgs::GameState_Phase_descriptor();
  const int num_phase_values = phase_enum_desc->value_count();
  NCursesMenuItem **phase_items = new NCursesMenuItem*[2 + num_phase_values];
  size_t max_length = 0;
  for (int i = 0; i < phase_enum_desc->value_count(); ++i) {
    SignalItem *item = new SignalItem(phase_enum_desc->value(i)->name());
    item->signal().connect(boost::bind(&LLSFRefBoxShell::set_game_phase, this,
				       phase_enum_desc->value(i)->name()));
    phase_items[i] = item;
    max_length = std::max(max_length, phase_enum_desc->value(i)->name().size());
  }

  s_cancel_phase_ = "** CANCEL **";
  size_t num_spaces =	(max_length > s_cancel_phase_.size())
		? (max_length - s_cancel_phase_.size()) / 2
		: 0;
  std::string spaces;
  for (size_t i = 0; i < num_spaces; ++i) spaces += " ";
  s_cancel_phase_ = spaces + s_cancel_phase_;
  phase_items[num_phase_values]   = new SignalItem(s_cancel_phase_);
  // termination sentinel element
  phase_items[num_phase_values+1] = new NCursesMenuItem();

  try {
    m_phase_ = new GenericItemsMenu(panel_, num_phase_values+1, phase_items);
    m_phase_->frame("Set Phase");
    m_phase_->hide();
  } catch (NCursesException &e) {
    logf("%s", e.message);
  }

  /*
  boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
  robots_[0]->set_name("Robot 1");
  robots_[0]->set_team("Carologistics");
  robots_[0]->set_last_seen(now);
  robots_[0]->refresh();
  */

  MessageRegister & message_register = client->message_register();
  message_register.add_message_type<llsf_msgs::GameState>();
  message_register.add_message_type<llsf_msgs::RobotInfo>();
  message_register.add_message_type<llsf_msgs::MachineInfo>();
  message_register.add_message_type<llsf_msgs::AttentionMessage>();
  message_register.add_message_type<llsf_msgs::OrderInfo>();
  message_register.add_message_type<llsf_log_msgs::LogMessage>();
  message_register.add_message_type<llsf_msgs::VersionInfo>();
  message_register.add_message_type<llsf_msgs::GameInfo>();

  client->signal_connected().connect(
    boost::bind(&LLSFRefBoxShell::dispatch_client_connected, this));
  client->signal_disconnected().connect(
    boost::bind(&LLSFRefBoxShell::dispatch_client_disconnected,
		this, boost::asio::placeholders::error));
  client->signal_received().connect(
    boost::bind(&LLSFRefBoxShell::dispatch_client_msg, this, _1, _2, _3));

  client->async_connect(cfg_refbox_host_.c_str(), cfg_refbox_port_);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBoxShell::handle_signal, this,
				 boost::asio::placeholders::error,
				 boost::asio::placeholders::signal_number));
#endif

  start_timers();
  start_keyboard();
  io_service_.run();

  return 0;
}


} // end of namespace llsfrb_shell

