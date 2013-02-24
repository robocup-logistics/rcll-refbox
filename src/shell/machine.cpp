
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

#include <cstring>
#include <unistd.h>

#include <cursesp.h>
#include <cursesf.h>

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <mutex>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellMachine::LLSFRefBoxShellMachine(std::string name, std::string type,
					       int begin_y, int begin_x)
  : NCursesPanel(1, 22, begin_y, begin_x),
    name_(name), type_(type)
{
}


LLSFRefBoxShellMachine::~LLSFRefBoxShellMachine()
{
  blink_state_.clear();
}

void
LLSFRefBoxShellMachine::set_type(std::string type)
{
  type_ = type;
}

void
LLSFRefBoxShellMachine::set_inputs(std::vector<llsf_msgs::PuckType> &inputs)
{
  inputs_ = inputs;
}


void
LLSFRefBoxShellMachine::set_lights(std::map<llsf_msgs::LightColor, llsf_msgs::LightState> &lights)
{
  lights_ = lights;
  blink_state_.clear();
  std::map<llsf_msgs::LightColor, llsf_msgs::LightState>::iterator l;
  for (l = lights.begin(); l != lights.end(); ++l) {
    if (l->second == llsf_msgs::BLINK) {
      blink_state_[l->first] = false;
    }
  }
}

void
LLSFRefBoxShellMachine::set_loaded_with(std::vector<llsf_msgs::PuckType> &loaded_with)
{
  loaded_with_ = loaded_with;
}


void
LLSFRefBoxShellMachine::set_puck_under_rfid(bool has_puck, llsf_msgs::PuckType puck_type)
{
  puck_under_rfid_ = has_puck;
  puck_under_rfid_type_ = puck_type;
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
  lights_.clear();
  blink_state_.clear();
  refresh();
}

int
LLSFRefBoxShellMachine::refresh()
{
  standend();
  erase();
  bkgd(' '|COLOR_PAIR(0));

  if (type_ == "") {
    addstr(0, 0, boost::str(boost::format("%-7s") % name_).c_str());
  } else {
    addstr(0, 0, boost::str(boost::format("%-3s|%-3s") % name_ % type_.substr(0,2)).c_str());
  }

  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_WHITE, COLOR_RED);
  init_pair(3, COLOR_WHITE, COLOR_YELLOW);
  init_pair(4, COLOR_WHITE, COLOR_GREEN);

  if (lights_.find(llsf_msgs::GREEN) != lights_.end() &&
      (lights_[llsf_msgs::GREEN] == llsf_msgs::ON ||
       (lights_[llsf_msgs::GREEN] == llsf_msgs::BLINK && blink_state_[llsf_msgs::GREEN])))
  {
    attron(' '|COLOR_PAIR(4));
  } else {
    attron(' '|COLOR_PAIR(1));
  }
  addstr( 0, 7, " ");
  if (lights_.find(llsf_msgs::YELLOW) != lights_.end() &&
      (lights_[llsf_msgs::YELLOW] == llsf_msgs::ON ||
       (lights_[llsf_msgs::YELLOW] == llsf_msgs::BLINK && blink_state_[llsf_msgs::YELLOW])))
  {
    attron(' '|COLOR_PAIR(3));
  } else {
    attron(' '|COLOR_PAIR(1));
  }
  addstr( 0, 8, " ");
  if (lights_.find(llsf_msgs::RED) != lights_.end() &&
      (lights_[llsf_msgs::RED] == llsf_msgs::ON ||
       (lights_[llsf_msgs::RED] == llsf_msgs::BLINK && blink_state_[llsf_msgs::RED])))
  {
    attron(' '|COLOR_PAIR(2));
  } else {
    attron(' '|COLOR_PAIR(1));
  }
  addstr( 0, 9, " ");

  if (puck_under_rfid_) {
    attron(' '|COLOR_PAIR(1)|A_BOLD);
    addstr(0, 11, llsf_msgs::PuckType_Name(puck_under_rfid_type_).c_str());
    attroff(A_BOLD);
  } else {
    attron(' '|COLOR_PAIR(1));
    addstr(0, 11, "  ");
  }

  int puck_x = 14;
  for (size_t i = 0; i < inputs_.size(); ++i) {
    bool puck_loaded =
      (std::find(loaded_with_.begin(), loaded_with_.end(), inputs_[i]) != loaded_with_.end());

    if (puck_loaded) {
      attron(' '|COLOR_PAIR(2)|A_BOLD);
    } else {
      attron(' '|COLOR_PAIR(1));
    }

    addstr( 0, puck_x, llsf_msgs::PuckType_Name(inputs_[i]).c_str());
    attroff(A_BOLD);
    puck_x += 3;
  }

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
