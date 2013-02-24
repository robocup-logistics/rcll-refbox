
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

#include <cursesp.h>

#include <boost/format.hpp>
#include <boost/date_time.hpp>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellRobot::LLSFRefBoxShellRobot(int begin_y, int begin_x)
  : NCursesPanel(2, 21, begin_y, begin_x)
{
}


void
LLSFRefBoxShellRobot::set_name(std::string name)
{
  name_ = name;
}

void
LLSFRefBoxShellRobot::set_team(std::string team)
{
  team_ = team;
}

void
LLSFRefBoxShellRobot::set_last_seen(boost::posix_time::ptime &last_seen)
{
  last_seen_ = last_seen;
}


int
LLSFRefBoxShellRobot::refresh()
{
  standend();
  if (name_ != "") {
    
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);

    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    boost::posix_time::time_duration td = now - last_seen_;
    long td_seconds = td.seconds();

    clear();
    
    if (td_seconds > 15) {
      bkgd(' '|COLOR_PAIR(0));
      name_ = "";
      team_ = "";
    } else if (td_seconds > 10) {
      bkgd(' '|COLOR_PAIR(2));
    } else if (td_seconds > 5) {
      bkgd(' '|COLOR_PAIR(3));
    } else {
      bkgd(' '|COLOR_PAIR(0));
    }
  }

  attron(A_BOLD);
  addstr(0, 1, name_.c_str());
  attroff(A_BOLD);
  addstr(1, 1, team_.c_str());

  return NCursesPanel::refresh();
}




} // end of namespace llsfrb_shell
