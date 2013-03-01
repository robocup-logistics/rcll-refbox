
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
#include <cursesm.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <cstring>
#include <unistd.h>

// defined in miliseconds
#define TIMER_INTERVAL 500
#define RECONNECT_TIMER_INTERVAL 1000
#define BLINK_TIMER_INTERVAL 250
#define ATTMSG_TIMER_INTERVAL 1000

using namespace protobuf_comm;

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class PassiveItem : public NCursesMenuItem
{
 public:
  PassiveItem(const char* text) : NCursesMenuItem(text) {
    options_off(O_SELECTABLE);
  }
};


class SignalItem : public NCursesMenuItem
{
public:
  SignalItem(const std::string &s) : NCursesMenuItem(s.c_str())
  {}

  boost::signals2::signal<void ()> & signal() { return signal_; }

  bool action() { signal_(); return TRUE; }

  private:
    boost::signals2::signal<void ()> signal_;
};

class Menu : public NCursesMenu
{
 public:
  Menu(NCursesWindow *parent, int n_items, NCursesMenuItem **items)
    : NCursesMenu(n_items + 2, max_cols(n_items, items) + 3,
		  (parent->lines() - max_cols(n_items, items))/2,
		  (parent->cols() - max_cols(n_items, items))/2),
      parent_(parent)
  {
    InitMenu(items, true, true);
    set_mark("");
  }

  virtual void On_Menu_Init()
  {
    bkgd(' '|COLOR_PAIR(0));
    //subWindow().bkgd(parent_->getbkgd());
    refresh();
  }

  int max_cols(int n_items, NCursesMenuItem **items)
  {
    int rv = 0;
    for (int i = 0; i < n_items; ++i) {
      rv = std::max(rv, (int)strlen(items[i]->name()));
    }
    return rv;
  }

private:
  NCursesWindow *parent_;
};


LLSFRefBoxShell::LLSFRefBoxShell()
  : quit_(false), error_(NULL), panel_(NULL), timer_(io_service_),
    reconnect_timer_(io_service_), try_reconnect_(true), blink_timer_(io_service_),
    attmsg_timer_(io_service_), attmsg_toggle_(true),
    stdin_(io_service_, dup(STDIN_FILENO))
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
  client = 0;
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

  delete m_state_;
  delete m_phase_;

  delete p_state_;
  delete p_phase_;
  delete p_time_;
  delete p_points_;
  delete p_attmsg_;
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
  stdin_.async_read_some(boost::asio::null_buffers(),
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
    rb_log_->refresh();
    int c = panel_->getch();
    if (c != ERR) {
      switch (c) {
      case ' ':
	if (s_state_ == "PAUSED") {
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
	break;
      case KEY_F(3):
	(*m_phase_)();
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

    rb_log_->refresh();
    p_attmsg_->refresh();
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
    client->async_connect("localhost", 4444);
  }
}


void
LLSFRefBoxShell::set_game_state(std::string state)
{
  llsf_msgs::GameState::State value;
  if (llsf_msgs::GameState_State_Parse(state, &value)) {
    rb_log_->printw("Requesting new game state %s\n", state.c_str());
    llsf_msgs::SetGameState msg;
    msg.set_state(value);
    try {
      client->send(msg);
    } catch (std::runtime_error &e) {
      rb_log_->printw("Sending game state failed: %s\n", e.what());
    }
  } else {
    rb_log_->printw("Failed to parse game state %s\n", state.c_str());
  }
}


void
LLSFRefBoxShell::set_game_phase(std::string phase)
{
  llsf_msgs::GameState::Phase value;
  if (llsf_msgs::GameState_Phase_Parse(phase, &value)) {
    rb_log_->printw("Requesting new game phase %s\n", phase.c_str());
    llsf_msgs::SetGamePhase msg;
    msg.set_phase(value);
    try {
      client->send(msg);
    } catch (std::runtime_error &e) {
      rb_log_->printw("Sending game phase failed: %s\n", e.what());
    }
  } else {
    rb_log_->printw("Failed to parse game phase %s\n", phase.c_str());
  }
}

void
LLSFRefBoxShell::client_connected()
{
  p_state_->clear();
  p_state_->addstr("CONNECTED");
  p_phase_->erase();
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
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
  p_state_->addstr("DISCONNECTED");
  //p_state_->addstr(error.message().c_str());

  if (! quit_) {
    std::lock_guard<std::mutex> lock(attmsg_mutex_);
    attmsg_string_ = "";
    p_attmsg_->bkgd(' '|COLOR_PAIR(0));
    p_attmsg_->erase();

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
  rb_log_->refresh();
  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
}

void
LLSFRefBoxShell::client_msg(uint16_t comp_id, uint16_t msg_type,
			    std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<llsf_msgs::GameState> g;
  if ((g = std::dynamic_pointer_cast<llsf_msgs::GameState>(msg))) {
    s_state_ = llsf_msgs::GameState::State_Name(g->state());
    p_state_->erase();
    p_state_->addstr(llsf_msgs::GameState::State_Name(g->state()).c_str());

    p_phase_->erase();
    p_phase_->addstr(llsf_msgs::GameState::Phase_Name(g->phase()).c_str());

    int min = g->timestamp().sec() / 60;
    int sec = g->timestamp().sec() - min * 60;
    p_time_->erase();
    p_time_->printw("%02d:%02d.%03ld", min, sec, g->timestamp().nsec() / 1000000);

    for (size_t i = 0; i < orders_.size(); ++i) {
      orders_[i]->set_gametime(g->timestamp().sec());
    }

    if (g->has_points()) {
      p_points_->erase();
      p_points_->printw("%u", g->points());
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

  std::shared_ptr<llsf_msgs::MachineInfo> minfo;
  if ((minfo = std::dynamic_pointer_cast<llsf_msgs::MachineInfo>(msg))) {
    for (int i = 0; i < minfo->machines_size(); ++i) {
      std::map<std::string, LLSFRefBoxShellMachine *>::iterator mpanel;
      const llsf_msgs::Machine &mspec = minfo->machines(i);
      //rb_log_->printw("Adding %s\n", mspec.name().c_str());
      if ((mpanel = machines_.find(mspec.name())) != machines_.end()) {
	mpanel->second->set_type(mspec.type());
	std::vector<llsf_msgs::PuckState> inputs(mspec.inputs_size());
	for (int j = 0; j < mspec.inputs_size(); ++j)  inputs[j] = mspec.inputs(j);
	mpanel->second->set_inputs(inputs);
	std::vector<llsf_msgs::PuckState> lw(mspec.loaded_with_size());
	for (int j = 0; j < mspec.loaded_with_size(); ++j) {
	  lw[j] = mspec.loaded_with(j).state();
	}
	mpanel->second->set_loaded_with(lw);
	std::map<llsf_msgs::LightColor, llsf_msgs::LightState> lights;
	for (int j = 0; j < mspec.lights_size(); ++j) {
	  const llsf_msgs::LightSpec &lspec = mspec.lights(j);
	  lights[lspec.color()] = lspec.state();
	}
	mpanel->second->set_lights(lights);
	if (mspec.has_puck_under_rfid()) {
	  mpanel->second->set_puck_under_rfid(true, mspec.puck_under_rfid().state());
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

  io_service_.dispatch(boost::bind(&LLSFRefBoxShell::refresh, this));
}

int
LLSFRefBoxShell::run()
{
  NCursesWindow rootw(::stdscr);
  panel_ = new NCursesPanel(rootw.lines() - 1, rootw.cols());
  navbar_ = new NCursesPanel(1, rootw.cols(), rootw.lines() - 1, 0);

  if (panel_->lines() < 30) {
    delete panel_;
    panel_ = NULL;
    error_ = "A minimum of 30 lines is required in the terminal";
    return -1;
  }

  curs_set(0); // invisible cursor
  use_default_colors();

  int height = panel_->maxy();

  panel_->bkgd(' '|COLOR_PAIR(0));
  panel_->frame();
  panel_->vline(1,                  panel_->width() - 26, height);
  panel_->addch(0,                  panel_->width() - 26, ACS_TTEE);
  panel_->addch(height, panel_->width() - 26, ACS_BTEE);

  panel_->hline(height-5, panel_->width() - 25, 24);
  panel_->addch(height-5, panel_->width() - 26, ACS_LTEE);
  panel_->addch(height-5, panel_->width() -  1, ACS_RTEE);

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
  panel_->addstr(height-5, panel_->width() - 15, "Game");
  panel_->addstr(rb_log_lines + 3, (panel_->width() - 26) / 2 - 3, "Orders");
  panel_->attroff(A_BOLD);

  panel_->attron(A_BOLD);
  panel_->addstr(height-4, panel_->width() - 24, "State:");
  panel_->addstr(height-3, panel_->width() - 24, "Phase:");
  panel_->addstr(height-2, panel_->width() - 24, "Time:");
  panel_->addstr(height-1, panel_->width() - 24, "Points:");
  panel_->attroff(A_BOLD);

  panel_->show();
  panel_->refresh();

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr("F2");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 3, "STATE");

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 10, "F3");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 13, "PHASE");

  navbar_->attron(' '|COLOR_PAIR(1)|A_BOLD);
  navbar_->addstr(0, 20, "F5");
  navbar_->standend();
  navbar_->attron(A_BOLD);
  navbar_->addstr(0, 23, "LOST PUCK");

  short default_fore, default_back;
  pair_content(0, &default_fore, &default_back);
  init_pair(200, COLOR_RED, default_back);
  init_pair(201, COLOR_WHITE, COLOR_RED);
  navbar_->attron(' '|COLOR_PAIR(201)|A_BOLD);
  navbar_->addstr(0, navbar_->cols() - 8, "SPC");
  navbar_->standend();
  navbar_->attron(' '|COLOR_PAIR(200)|A_BOLD);
  navbar_->addstr(0, navbar_->cols() - 4, "STOP");
  navbar_->refresh();

  rb_log_ = new NCursesPanel(rb_log_lines,  panel_->width() - 28, 3, 1);
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
  p_state_  = new NCursesPanel(1, 15, height-4,  panel_->width() - 16);
  p_phase_  = new NCursesPanel(1, 15, height-3,  panel_->width() - 16);
  p_time_   = new NCursesPanel(1, 15, height-2,  panel_->width() - 16);
  p_points_ = new NCursesPanel(1, 15, height-1,  panel_->width() - 16);

  p_state_->addstr("DISCONNECTED");
  p_points_->addstr("0");

  // State menu
  const google::protobuf::EnumDescriptor *state_enum_desc =
    llsf_msgs::GameState_State_descriptor();
  const int num_state_values = state_enum_desc->value_count();
  NCursesMenuItem **state_items = new NCursesMenuItem*[2 + num_state_values];
  for (int i = 0; i < state_enum_desc->value_count(); ++i) {
    SignalItem *item = new SignalItem(state_enum_desc->value(i)->name());
    item->signal().connect(boost::bind(&LLSFRefBoxShell::set_game_state, this,
				       state_enum_desc->value(i)->name()));
    state_items[i] = item;
  }
  // termination sentinel element
  s_cancel_ = "** CANCEL **";
  state_items[num_state_values]   = new SignalItem(s_cancel_);
  state_items[num_state_values+1] = new NCursesMenuItem();

  try {
    m_state_ = new Menu(panel_, num_state_values+1, state_items);
    m_state_->frame("Set State");
    m_state_->hide();
  } catch (NCursesException &e) {
    rb_log_->printw("%s\n", e.message);
  }

  // Phase menu
  const google::protobuf::EnumDescriptor *phase_enum_desc =
    llsf_msgs::GameState_Phase_descriptor();
  const int num_phase_values = phase_enum_desc->value_count();
  NCursesMenuItem **phase_items = new NCursesMenuItem*[2 + num_phase_values];
  for (int i = 0; i < phase_enum_desc->value_count(); ++i) {
    SignalItem *item = new SignalItem(phase_enum_desc->value(i)->name());
    item->signal().connect(boost::bind(&LLSFRefBoxShell::set_game_phase, this,
				       phase_enum_desc->value(i)->name()));
    phase_items[i] = item;
  }
  // termination sentinel element
  s_cancel_ = "** CANCEL **";
  phase_items[num_phase_values]   = new SignalItem(s_cancel_);
  phase_items[num_phase_values+1] = new NCursesMenuItem();

  try {
    m_phase_ = new Menu(panel_, num_phase_values+1, phase_items);
    m_phase_->frame("Set Phase");
    m_phase_->hide();
  } catch (NCursesException &e) {
    rb_log_->printw("%s\n", e.message);
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
  start_keyboard();
  io_service_.run();

  return 0;
}


} // end of namespace llsfrb_shell

