
/***************************************************************************
 *  robot.cpp - LLSF RefBox shell - robot
 *
 *  Created: Thu Feb 21 22:37:47 2013
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

#include "robot.h"
#include "colors.h"

#include <cursesp.h>

#include <boost/format.hpp>
#include <boost/date_time.hpp>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellRobot::LLSFRefBoxShellRobot(int begin_y, int begin_x)
  : NCursesPanel(2, 24, begin_y, begin_x)
{
}


void
LLSFRefBoxShellRobot::update(unsigned int number, std::string name,
			     std::string team, std::string host,
			     llsf_msgs::RobotState state, float maintenance_time_remaining,
			     unsigned int maintenance_cycles)
{
  number_ = number;
  name_   = name;
  team_   = team;
  host_   = host;
  state_  = state;
  maintenance_time_remaining_ = maintenance_time_remaining;
  maintenance_cycles_         = maintenance_cycles;
}

void
LLSFRefBoxShellRobot::set_last_seen(boost::posix_time::ptime &last_seen)
{
  last_seen_ = last_seen;
}

void
LLSFRefBoxShellRobot::reset()
{
  number_ = 0;
  name_ = team_ = host_ = "";
  refresh();
}


int
LLSFRefBoxShellRobot::refresh()
{
  standend();
  if (name_ != "") {

    boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
    boost::posix_time::time_duration td = now - last_seen_;
    long td_seconds = td.total_seconds();

    std::string line_1 =
      boost::str(boost::format("%u %s (%s)") % number_ % name_ % team_).substr(0, width());;
    for (int i = line_1.length(); i < width(); ++i) {
      line_1 += " ";
    }

    erase();

    if (td_seconds >= 10) {
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
    } else if (td_seconds >= 5) {
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_YELLOW));
    } else {
      attron(' '|COLOR_PAIR(COLOR_DEFAULT));
    }
    attron(A_BOLD);
    printw(0, 0, "%s", line_1.c_str());
    attroff(A_BOLD);
    standend();

    addstr(1, 0, host_.c_str());
    attron(A_BOLD);
    if (state_ == llsf_msgs::MAINTENANCE) {
      if (maintenance_time_remaining_ < 1) {
	if ((-(int)truncf(maintenance_time_remaining_) % 2) == 0) {
	  attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK));
	} else {
	  attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
	}
      } else if (maintenance_time_remaining_ < 16) {
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
      } else if (maintenance_time_remaining_ < 31) {
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_YELLOW));
      }
      printw(1, width() - 10, "%3is", (int)truncf(maintenance_time_remaining_));
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_YELLOW));
    } else if (state_ == llsf_msgs::DISQUALIFIED) {
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
    }
    printw(1, width() - 5, "%s", RobotState_Name(state_).substr(0,3).c_str());
    standend();

    if (maintenance_cycles_ > 0) {
      attron(A_BOLD);
      attron(' '|COLOR_PAIR(COLOR_RED_ON_BACK));
      //mvwaddwstr(w, 1, width() - 1, L"\u26a0");
      printw(1, width() - 1, "!");
      standend();
    }

  } else {
    bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
    erase();
  }

  return NCursesPanel::refresh();
}




} // end of namespace llsfrb_shell
