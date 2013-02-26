
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

#include <protobuf_comm/client.h>

#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/MachineSpec.pb.h>
#include <msgs/AttentionMessage.pb.h>
#include <msgs/OrderInstruction.pb.h>

#include <cursesp.h>
#include <cursesf.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <cstring>
#include <unistd.h>

// defined in miliseconds
#define TIMER_INTERVAL 500
#define RECONNECT_TIMER_INTERVAL 2000
#define BLINK_TIMER_INTERVAL 250
#define ATTMSG_TIMER_INTERVAL 1000

using namespace protobuf_comm;

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShell::LLSFRefBoxShell(int argc, char **argv)
  : NCursesApplication(/*wantColors*/ TRUE),
    quit_(false), error_(NULL), panel_(NULL), timer_(io_service_),
    reconnect_timer_(io_service_), try_reconnect_(true), blink_timer_(io_service_),
    attmsg_timer_(io_service_), attmsg_toggle_(true)
{
  client = new ProtobufStreamClient();
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

  delete client;
  delete panel_;

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

  delete p_state_;
  delete p_phase_;
  delete p_time_;
  delete p_points_;
  delete p_attmsg_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}

void
LLSFRefBoxShell::title()
{
  const char * const titleText = "LLSF RefBox Shell";
  const int len = ::strlen(titleText);

  if (titleWindow) {
    titleWindow->bkgd(screen_titles());
    titleWindow->addstr(0, (titleWindow->cols() - len)/2, titleText);
    titleWindow->noutrefresh();
  }
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
      attmsg_toggle_ = ! attmsg_toggle_;
      short default_fore, default_back;
      pair_content(0, &default_fore, &default_back);
      init_pair(200, COLOR_RED, default_back);
      init_pair(201, COLOR_WHITE, COLOR_RED);

      if (attmsg_toggle_) {
	p_attmsg_->bkgd(' '|COLOR_PAIR(200));
	p_attmsg_->attron(' '|COLOR_PAIR(200)|A_BOLD);
      } else {
	p_attmsg_->bkgd(' '|COLOR_PAIR(201));
	p_attmsg_->attron(' '|COLOR_PAIR(201)|A_BOLD);
      }
      if ((int)attmsg_string_.length() >= p_attmsg_->width()) {
	p_attmsg_->addstr(attmsg_string_.c_str());
      } else {
	p_attmsg_->addstr(0, (p_attmsg_->width() - attmsg_string_.length()) / 2,
			  attmsg_string_.c_str());
      }

      attmsg_timer_.expires_at(attmsg_timer_.expires_at()
			       + boost::posix_time::milliseconds(ATTMSG_TIMER_INTERVAL));
      attmsg_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_attmsg_timer, this,
					   boost::asio::placeholders::error));      
    } else {
      p_attmsg_->erase();
    }
  }
}


/** Handle reconnect timer event.
 * @param error error code
 */
void
LLSFRefBoxShell::handle_reconnect_timer(const boost::system::error_code& error)
{
  if (! error && try_reconnect_ && ! quit_) {
    client->async_connect("localhost", 4444);
  }
}


void
LLSFRefBoxShell::client_connected()
{
  p_state_->clear();
  p_state_->addstr("CONNECTED");
}

void
LLSFRefBoxShell::client_disconnected(const boost::system::error_code &error)
{
  p_state_->erase();
  p_phase_->erase();
  p_time_->erase();
  p_points_->erase();
  p_state_->addstr("DISCONNECTED");
  //p_state_->addstr(error.message().c_str());

  if (! quit_) {
    std::lock_guard<std::mutex> lock(attmsg_mutex_);
    attmsg_string_ = "";
    p_attmsg_->bkgd(' '|COLOR_PAIR(0));
    p_attmsg_->erase();
    p_attmsg_->refresh();

    std::map<std::string, LLSFRefBoxShellMachine *>::iterator m;
    for (m = machines_.begin(); m != machines_.end(); ++m) {
      m->second->reset();
    }

    for (size_t i = 0; i < orders_.size(); ++i) {
      orders_[i]->reset();
    }

    if (try_reconnect_) {
      reconnect_timer_.expires_from_now(boost::posix_time::milliseconds(RECONNECT_TIMER_INTERVAL));
      reconnect_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_reconnect_timer, this,
					      boost::asio::placeholders::error));
    }
  }

}

void
LLSFRefBoxShell::client_msg(uint16_t comp_id, uint16_t msg_type,
			    std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<llsf_msgs::GameState> g;
  if ((g = std::dynamic_pointer_cast<llsf_msgs::GameState>(msg))) {
    p_state_->clear();
    p_state_->addstr(llsf_msgs::GameState::State_Name(g->state()).c_str());

    int min = g->timestamp().sec() / 60;
    int sec = g->timestamp().sec() - min * 60;
    p_time_->erase();
    p_time_->printw("%02d:%02d.%03ld", min, sec, g->timestamp().nsec() / 1000000);

    for (size_t i = 0; i < orders_.size(); ++i) {
      orders_[i]->set_gametime(g->timestamp().sec());
    }
  }

  std::shared_ptr<llsf_msgs::RobotInfo> r;
  if ((r = std::dynamic_pointer_cast<llsf_msgs::RobotInfo>(msg))) {
    for (int i = 0; i < r->robots_size(); ++i) {
      // more robots than we can show
      if ((size_t)i >= robots_.size()) break;

      const llsf_msgs::Robot &robot = r->robots(i);
      robots_[i]->set_name(robot.name());
      robots_[i]->set_team(robot.team());
      boost::posix_time::ptime
	last_seen(boost::posix_time::from_time_t(robot.last_seen().sec()));
      last_seen += boost::posix_time::nanoseconds(robot.last_seen().nsec());
      robots_[i]->set_last_seen(last_seen);
      robots_[i]->refresh();
    }
  }

  std::shared_ptr<llsf_msgs::MachineSpecs> mspecs;
  if ((mspecs = std::dynamic_pointer_cast<llsf_msgs::MachineSpecs>(msg))) {
    for (int i = 0; i < mspecs->machines_size(); ++i) {
      std::map<std::string, LLSFRefBoxShellMachine *>::iterator mpanel;
      const llsf_msgs::MachineSpec &mspec = mspecs->machines(i);
      //rb_log_->printw("Adding %s\n", mspec.name().c_str());
      if ((mpanel = machines_.find(mspec.name())) != machines_.end()) {
	mpanel->second->set_type(mspec.type());
	std::vector<llsf_msgs::PuckType> inputs(mspec.inputs_size());
	for (int j = 0; j < mspec.inputs_size(); ++j)  inputs[j] = mspec.inputs(j);
	mpanel->second->set_inputs(inputs);
	std::vector<llsf_msgs::PuckType> lw(mspec.loaded_with_size());
	for (int j = 0; j < mspec.loaded_with_size(); ++j)  lw[j] = mspec.loaded_with(j);
	mpanel->second->set_loaded_with(lw);
	std::map<llsf_msgs::LightColor, llsf_msgs::LightState> lights;
	for (int j = 0; j < mspec.lights_size(); ++j) {
	  const llsf_msgs::LightSpec &lspec = mspec.lights(j);
	  lights[lspec.color()] = lspec.state();
	}
	mpanel->second->set_lights(lights);
	if (mspec.has_puck_under_rfid()) {
	  mpanel->second->set_puck_under_rfid(true, mspec.puck_under_rfid());
	} else {
	  mpanel->second->set_puck_under_rfid(false);
	}
	mpanel->second->refresh();
      }
    }
  }

  std::shared_ptr<llsf_msgs::AttentionMessage> am;
  if ((am = std::dynamic_pointer_cast<llsf_msgs::AttentionMessage>(msg))) {
    std::lock_guard<std::mutex> lock(attmsg_mutex_);
    attmsg_string_ = am->message();
    attmsg_has_endtime_ = am->has_time_to_show();
    if (attmsg_has_endtime_) {
      boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
      attmsg_endtime_ = now + boost::posix_time::seconds(am->time_to_show());
    }
    if (attmsg_string_ == "") {
      attmsg_timer_.cancel();
    } else {
      attmsg_toggle_ = true;
      attmsg_timer_.expires_from_now(boost::posix_time::milliseconds(0));
      attmsg_timer_.async_wait(boost::bind(&LLSFRefBoxShell::handle_attmsg_timer, this,
					   boost::asio::placeholders::error));
    }
  }

  std::shared_ptr<llsf_msgs::OrderInstruction> ordins;
  if ((ordins = std::dynamic_pointer_cast<llsf_msgs::OrderInstruction>(msg))) {
    const size_t size = std::min((size_t)ordins->orders_size(), (size_t)orders_.size());
    for (size_t i = 0; i < size; ++i) {
      const llsf_msgs::OrderSpec &ospec = ordins->orders(i);
      orders_[i]->update(ospec.id(), ospec.product(), ospec.quantity_requested(),
			 ospec.quantity_delivered(), ospec.delivery_period_begin(),
			 ospec.delivery_period_end(), ospec.delivery_gate());
      orders_[i]->refresh();
    }
  }
}

int
LLSFRefBoxShell::run()
{
  panel_ = new NCursesPanel();

  if (panel_->lines() < 30) {
    delete panel_;
    panel_ = NULL;
    error_ = "A minimum of 30 lines is required in the terminal";
    return -1;
  }

  curs_set(0); // invisible cursor
  use_default_colors();

  panel_->bkgd(' '|COLOR_PAIR(0));
  panel_->frame();
  panel_->vline(1,                  panel_->width() - 26, panel_->height()-2);
  panel_->addch(0,                  panel_->width() - 26, ACS_TTEE);
  panel_->addch(panel_->height()-1, panel_->width() - 26, ACS_BTEE);

  panel_->hline(panel_->height()-6, panel_->width() - 25, 24);
  panel_->addch(panel_->height()-6, panel_->width() - 26, ACS_LTEE);
  panel_->addch(panel_->height()-6, panel_->width() -  1, ACS_RTEE);

  panel_->hline(17, panel_->width() - 25, 24);
  panel_->addch(17, panel_->width() - 26, ACS_LTEE);
  panel_->addch(17, panel_->width() -  1, ACS_RTEE);

  panel_->hline(2, 1, panel_->width() - 26);
  panel_->addch(2, 0, ACS_LTEE);
  panel_->addch(2, panel_->width() - 26, ACS_RTEE);

  int rb_log_lines   = panel_->maxy() - 10;

  panel_->hline(rb_log_lines + 3, 1, panel_->width() - 26);
  panel_->addch(rb_log_lines + 3, 0, ACS_LTEE);
  panel_->addch(rb_log_lines + 3, panel_->width() - 26, ACS_RTEE);

  panel_->attron(A_BOLD);
  panel_->addstr(0, panel_->width() - 17, "Machines");
  panel_->addstr(0, (panel_->width() - 26) / 2 - 7, "Attention Message");
  panel_->addstr(2, (panel_->width() - 26) / 2 - 4, "RefBox Log");
  panel_->addstr(17, panel_->width() - 16, "Robots");
  panel_->addstr(panel_->height()-6, panel_->width() - 15, "Game");
  panel_->addstr(rb_log_lines + 3, (panel_->width() - 26) / 2 - 3, "Orders");
  panel_->attroff(A_BOLD);

  panel_->attron(A_BOLD);
  panel_->addstr(panel_->height()-5, panel_->width() - 24, "State:");
  panel_->addstr(panel_->height()-4, panel_->width() - 24, "Phase:");
  panel_->addstr(panel_->height()-3, panel_->width() - 24, "Time:");
  panel_->addstr(panel_->height()-2, panel_->width() - 24, "Points:");
  panel_->attroff(A_BOLD);

  rb_log_ = new NCursesPanel(rb_log_lines,  panel_->width() - 28,
			     3, 1);
  rb_log_->scrollok(TRUE);

  const int mx = panel_->width() - 24;
  machines_["M1"]   = new LLSFRefBoxShellMachine("M1",  "?",  1, mx);
  machines_["M2"]   = new LLSFRefBoxShellMachine("M2",  "?",  2, mx);
  machines_["M3"]   = new LLSFRefBoxShellMachine("M3",  "?",  3, mx);
  machines_["M4"]   = new LLSFRefBoxShellMachine("M4",  "?",  4, mx);
  machines_["M5"]   = new LLSFRefBoxShellMachine("M5",  "?",  5, mx);
  machines_["M6"]   = new LLSFRefBoxShellMachine("M6",  "?",  6, mx);
  machines_["M7"]   = new LLSFRefBoxShellMachine("M7",  "?",  7, mx);
  machines_["M8"]   = new LLSFRefBoxShellMachine("M8",  "?",  8, mx);
  machines_["M9"]   = new LLSFRefBoxShellMachine("M9",  "?",  9, mx);
  machines_["M10"]  = new LLSFRefBoxShellMachine("M10", "?", 10, mx);
  machines_["D1"]   = new LLSFRefBoxShellMachine("D1",   "", 11, mx);
  machines_["D2"]   = new LLSFRefBoxShellMachine("D2",   "", 12, mx);
  machines_["D3"]   = new LLSFRefBoxShellMachine("D3",   "", 13, mx);
  machines_["TST"]  = new LLSFRefBoxShellMachine("TST",  "", 14, mx);
  machines_["R1"]   = new LLSFRefBoxShellMachine("R1",   "", 15, mx);
  machines_["R2"]   = new LLSFRefBoxShellMachine("R2",   "", 16, mx);

  std::map<std::string, LLSFRefBoxShellMachine *>::iterator m;
  for (m = machines_.begin(); m != machines_.end(); ++m) {
    m->second->refresh();
  }

  robots_.resize(3, NULL);
  for (size_t i = 0; i < robots_.size(); ++i) {
    robots_[i] = new LLSFRefBoxShellRobot(18 + 2 * i, panel_->width() - 25);
    robots_[i]->refresh();
  }

  orders_.resize(12, NULL);
  for (size_t i = 0; i < orders_.size(); ++i) {
    orders_[i] = new LLSFRefBoxShellOrder(rb_log_lines + 4 + i / 2,
					  (i % 2) ? ((panel_->width() - 22) / 2) : 1);
    orders_[i]->refresh();
  }

  panel_->refresh();

  p_attmsg_ = new NCursesPanel(1, panel_->width() - 27, 1, 1);
  p_state_  = new NCursesPanel(1, 15, panel_->height()-5,  panel_->width() - 16);
  p_phase_  = new NCursesPanel(1, 15, panel_->height()-4,  panel_->width() - 16);
  p_time_   = new NCursesPanel(1, 15, panel_->height()-3,  panel_->width() - 16);
  p_points_ = new NCursesPanel(1, 15, panel_->height()-2,  panel_->width() - 16);

  p_state_->addstr("DISCONNECTED");
  p_points_->addstr("0");

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
  message_register.add_message_type<llsf_msgs::MachineSpecs>();
  message_register.add_message_type<llsf_msgs::AttentionMessage>();
  message_register.add_message_type<llsf_msgs::OrderInstruction>();

  client->signal_connected().connect(boost::bind(&LLSFRefBoxShell::client_connected, this));
  client->signal_disconnected().connect(boost::bind(&LLSFRefBoxShell::client_disconnected,
						    this, boost::asio::placeholders::error));
  client->signal_received().connect(boost::bind(&LLSFRefBoxShell::client_msg, this,
						_1, _2, _3));
  client->async_connect("localhost", 4444);

  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBoxShell::handle_signal, this,
				 boost::asio::placeholders::error,
				 boost::asio::placeholders::signal_number));

  start_timers();
  io_service_.run();

  return 0;
}


} // end of namespace llsfrb_shell

