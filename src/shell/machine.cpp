
/***************************************************************************
 *  machine.cpp - LLSF RefBox shell - machine
 *
 *  Created: Thu Feb 21 18:10:13 2013
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

#include "machine.h"
#include "colors.h"

#include <cstring>
#include <unistd.h>

#include <cursesp.h>

#include <boost/format.hpp>
#include <mutex>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellMachine::LLSFRefBoxShellMachine(std::string name, std::string type,
					       int begin_y, int begin_x, bool visible)
  : NCursesPanel(1, 28, begin_y, begin_x),
    visible_(visible)
{
  set_visible(visible_);
}


LLSFRefBoxShellMachine::~LLSFRefBoxShellMachine()
{
  blink_state_.clear();
}

void
LLSFRefBoxShellMachine::set_visible(bool visible)
{
  visible_ = visible;
  if (visible) {
    show();
  } else {
    hide();
  }
}

void
LLSFRefBoxShellMachine::update(const llsf_msgs::Machine &minfo)
{
  minfo_ = std::make_shared<llsf_msgs::Machine>(minfo);

  std::map<llsf_msgs::LightColor, llsf_msgs::LightState> lights;
  for (int j = 0; j < minfo_->lights_size(); ++j) {
    const llsf_msgs::LightSpec &lspec = minfo_->lights(j);
    lights[lspec.color()] = lspec.state();
  }

  if (lights_ != lights) {
    lights_ = lights;
    blink_state_.clear();
    std::map<llsf_msgs::LightColor, llsf_msgs::LightState>::iterator l;
    for (l = lights.begin(); l != lights.end(); ++l) {
      if (l->second == llsf_msgs::BLINK) {
	blink_state_[l->first] = false;
      }
    }
  }
}

void
LLSFRefBoxShellMachine::flip_blink_states()
{
  if (! blink_state_.empty()) {
    std::map<llsf_msgs::LightColor, bool>::iterator l;
    for (l = blink_state_.begin(); l != blink_state_.end(); ++l) {
      l->second = ! l->second;
    }
    refresh();
  }
}

void
LLSFRefBoxShellMachine::reset()
{
  minfo_.reset();
  lights_.clear();
  blink_state_.clear();
  refresh();
}

int
LLSFRefBoxShellMachine::refresh()
{
  if (! visible_)  return OK;

  standend();
  erase();
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));

  std::string name;
  std::string type;
  std::string zone;
  std::string rotation;

  if (minfo_) {
    name = minfo_->name();
    type = minfo_->type();
    zone = minfo_->has_zone() ? llsf_msgs::Zone_Name(minfo_->zone()) : "";
    rotation = "";
    if (minfo_->has_rotation()) {
      unsigned int rotation_i = minfo_->rotation();
      if ( rotation_i == 0   ) rotation = "→";
      else if ( rotation_i == 45  ) rotation = "↗";
      else if ( rotation_i == 90  ) rotation = "↑";
      else if ( rotation_i == 135 ) rotation = "↖";
      else if ( rotation_i == 180 ) rotation = "←";
      else if ( rotation_i == 225 ) rotation = "↙";
      else if ( rotation_i == 270 ) rotation = "↓";
      else if ( rotation_i == 315 ) rotation = "↘";
      else rotation = "?";
    }
  }

  if (name.find("C-") == 0) {
    attron(' '|COLOR_PAIR(COLOR_CYAN_ON_BACK));
  } else if (name.find("M-") == 0) {
    attron(' '|COLOR_PAIR(COLOR_MAGENTA_ON_BACK));
  }
  attron(' '|A_BOLD);
  addstr(0, 0, boost::str(boost::format("%-5s") % name).c_str());
  attroff(A_BOLD);
  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));

  addstr(0, 5, boost::str(boost::format("|%-2s|%3s|%1s") % type % zone % rotation).c_str());

  if (lights_.find(llsf_msgs::GREEN) != lights_.end() &&
      (lights_[llsf_msgs::GREEN] == llsf_msgs::ON ||
       (lights_[llsf_msgs::GREEN] == llsf_msgs::BLINK && blink_state_[llsf_msgs::GREEN])))
  {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN));
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  }
  addstr( 0, 17, " ");
  if (lights_.find(llsf_msgs::YELLOW) != lights_.end() &&
      (lights_[llsf_msgs::YELLOW] == llsf_msgs::ON ||
       (lights_[llsf_msgs::YELLOW] == llsf_msgs::BLINK && blink_state_[llsf_msgs::YELLOW])))
  {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW));
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  }
  addstr( 0, 18, " ");
  if (lights_.find(llsf_msgs::RED) != lights_.end() &&
      (lights_[llsf_msgs::RED] == llsf_msgs::ON ||
       (lights_[llsf_msgs::RED] == llsf_msgs::BLINK && blink_state_[llsf_msgs::RED])))
  {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  }
  addstr( 0, 19, " ");


  bool expl = false;
  if (minfo_ && minfo_->has_exploration_zone_state()) {
	  expl = true;
	  switch (minfo_->exploration_zone_state()) {
	  case llsf_msgs::NO_REPORT:
		  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
		  addstr(0, 21, " ");
		  break;
	  case llsf_msgs::CORRECT_REPORT:
		  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN) | A_BOLD);
		  addstr(0, 21, "+");
		  attroff(A_BOLD);
		  break;
	  case llsf_msgs::WRONG_REPORT:
		  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED) | A_BOLD);
		  addstr(0, 21, "-");
		  attroff(A_BOLD);
		  break;
	  }
  }
  if (minfo_ && minfo_->has_exploration_rotation_state()) {
	  expl = true;
	  switch (minfo_->exploration_rotation_state()) {
	  case llsf_msgs::NO_REPORT:
		  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
		  addstr(0, 22, " ");
		  break;
	  case llsf_msgs::CORRECT_REPORT:
		  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN) | A_BOLD);
		  addstr(0, 22, "+");
		  attroff(A_BOLD);
		  break;
	  case llsf_msgs::WRONG_REPORT:
		  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED) | A_BOLD);
		  addstr(0, 22, "-");
		  attroff(A_BOLD);
		  break;
	  }
  }
  if (! expl) {
	  if (! minfo_ || minfo_->state().empty()) {
		  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
		  addstr(0, 21, "  ");
	  } else {
		  if (minfo_->state() == "BROKEN" || minfo_->state() == "DOWN") {
			  attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK));
			  addstr(0, 21, minfo_->state().substr(0,2).c_str());
		  } else if (minfo_->state() == "PREPARED") {
			  attron(' '|COLOR_PAIR(COLOR_GREEN_ON_BACK));
			  addstr(0, 21, "PP");
		  } else if (minfo_->state() == "PROCESSING" || minfo_->state() == "PROCESSED") {
			  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN));
			  addstr(0, 21, "P");
			  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW));
			  addstr(0, 22, minfo_->state().substr(minfo_->state().length()-1, 1).c_str());
		  } else if (minfo_->state() == "READY-AT-OUTPUT") {
			  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW));
			  addstr(0, 21, "RO");
		  } else {
			  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
			  addstr(0, 21, minfo_->state().substr(0,2).c_str());
		  }
	  }
  }

  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  if (minfo_ && minfo_->loaded_with() > 0) {
    addstr(0, 24, (boost::str(boost::format("%u") % minfo_->loaded_with())).c_str());
  } else {
    addstr(0, 24, " ");
  }

  if (type == "RS") {
    if (minfo_ && minfo_->ring_colors_size() > 0) {
      for (int i = 0; i < minfo_->ring_colors_size(); ++i) {
	switch (minfo_->ring_colors(i)) {
	case llsf_msgs::RING_BLUE:
	  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLUE)|A_BOLD); break;
	case llsf_msgs::RING_GREEN:
	  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN)|A_BOLD); break;
	case llsf_msgs::RING_ORANGE:
	  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_ORANGE)|A_BOLD); break;
	case llsf_msgs::RING_YELLOW:
	  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW)|A_BOLD); break;
	}
	if (minfo_->has_instruction_rs() &&
	    minfo_->instruction_rs().ring_color() == minfo_->ring_colors(i)) {
	  addstr(0, 26+i, "*");
	} else {
	  addstr(0, 26+i, " ");
	}
	attroff(A_BOLD);
      }

      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
      for (int j = minfo_->ring_colors_size(); j <= 2; ++j) {
	addstr(0, 26+j, " ");
      }

    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
      addstr(0, 26, "  ");
    }
  } else if (type == "BS") {
    if (minfo_ && minfo_->has_instruction_bs()) {
      switch (minfo_->instruction_bs().color()) {
      case llsf_msgs::BASE_RED:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));   break;
      case llsf_msgs::BASE_SILVER:
	attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE)); break;
      case llsf_msgs::BASE_BLACK:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLACK));   break;
      }
      addstr(0, 26, " ");

      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
      switch (minfo_->instruction_bs().side()) {
      case llsf_msgs::INPUT:  addstr(0, 27, "I"); break;
      case llsf_msgs::OUTPUT: addstr(0, 27, "O"); break;
      }
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
      addstr(0, 26, "  ");
    }
  } else if (type == "DS") {
    if (minfo_ && minfo_->has_instruction_ds()) {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
      addstr(0, 26, (boost::str(boost::format("%u")
				% minfo_->instruction_ds().order_id())).c_str());
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
      addstr(0, 26, "  ");
    }
  } else if (type == "CS") {
    if (minfo_ && minfo_->has_instruction_cs()) {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
      switch (minfo_->instruction_cs().operation()) {
      case llsf_msgs::RETRIEVE_CAP: addstr(0, 26, "RC"); break;
      case llsf_msgs::MOUNT_CAP:    addstr(0, 26, "MC"); break;
      }
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
      addstr(0, 26, "  ");
    }	
  }

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
