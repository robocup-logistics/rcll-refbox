
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
  : NCursesPanel(1, 22, begin_y, begin_x),
    visible_(visible), name_(name), type_(type), puck_under_rfid_(false),
    has_correctly_reported_field_(false), correctly_reported_(false)
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
LLSFRefBoxShellMachine::set_type(std::string type)
{
  type_ = type;
}

void
LLSFRefBoxShellMachine::set_inputs(std::vector<llsf_msgs::PuckState> &inputs)
{
  inputs_ = inputs;
}


void
LLSFRefBoxShellMachine::set_lights(std::map<llsf_msgs::LightColor, llsf_msgs::LightState> &lights)
{
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
LLSFRefBoxShellMachine::set_loaded_with(std::vector<llsf_msgs::PuckState> &loaded_with)
{
  loaded_with_ = loaded_with;
}


void
LLSFRefBoxShellMachine::set_puck_under_rfid(bool has_puck, llsf_msgs::PuckState puck_state)
{
  puck_under_rfid_ = has_puck;
  puck_under_rfid_state_ = puck_state;
}

void
LLSFRefBoxShellMachine::set_correctly_reported(bool has_field, bool correctly_reported)
{
  has_correctly_reported_field_ = has_field;
  correctly_reported_ = correctly_reported;
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
  type_ = "?";
  inputs_.clear();
  loaded_with_.clear();
  puck_under_rfid_ = false;
  has_correctly_reported_field_ = false;
  correctly_reported_ = false;
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

  if (type_ == "") {
    addstr(0, 0, boost::str(boost::format("%-7s") % name_).c_str());
  } else {
    addstr(0, 0, boost::str(boost::format("%-3s|%-3s") % name_ % type_.substr(0,2)).c_str());
  }

  if (lights_.find(llsf_msgs::GREEN) != lights_.end() &&
      (lights_[llsf_msgs::GREEN] == llsf_msgs::ON ||
       (lights_[llsf_msgs::GREEN] == llsf_msgs::BLINK && blink_state_[llsf_msgs::GREEN])))
  {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN));
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  }
  addstr( 0, 7, " ");
  if (lights_.find(llsf_msgs::YELLOW) != lights_.end() &&
      (lights_[llsf_msgs::YELLOW] == llsf_msgs::ON ||
       (lights_[llsf_msgs::YELLOW] == llsf_msgs::BLINK && blink_state_[llsf_msgs::YELLOW])))
  {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_YELLOW));
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  }
  addstr( 0, 8, " ");
  if (lights_.find(llsf_msgs::RED) != lights_.end() &&
      (lights_[llsf_msgs::RED] == llsf_msgs::ON ||
       (lights_[llsf_msgs::RED] == llsf_msgs::BLINK && blink_state_[llsf_msgs::RED])))
  {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  }
  addstr( 0, 9, " ");

  if (puck_under_rfid_) {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE)|A_BOLD);
    addstr(0, 11, llsf_msgs::PuckState_Name(puck_under_rfid_state_).substr(0,2).c_str());
    attroff(A_BOLD);
  } else if (has_correctly_reported_field_) {
    attron(' '|COLOR_PAIR(correctly_reported_ ? COLOR_WHITE_ON_GREEN : COLOR_WHITE_ON_RED)|A_BOLD);
    addstr(0, 11, correctly_reported_ ? "++" : "--");
    attroff(A_BOLD);
  } else {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    addstr(0, 11, "  ");
  }

  int puck_x = 14;
  for (size_t i = 0; i < inputs_.size(); ++i) {
    bool puck_loaded =
      (std::find(loaded_with_.begin(), loaded_with_.end(), inputs_[i]) != loaded_with_.end());

    if (puck_loaded) {
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED)|A_BOLD);
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    }

    addstr( 0, puck_x, llsf_msgs::PuckState_Name(inputs_[i]).substr(0,2).c_str());
    attroff(A_BOLD);
    puck_x += 3;
  }

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
