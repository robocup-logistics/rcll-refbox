
/***************************************************************************
 *  puck.cpp - LLSF RefBox shell - puck
 *
 *  Created: Fri Mar 01 15:54:22 2013
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

#include "puck.h"

#include <cstring>
#include <unistd.h>

#include <cursesp.h>

#include "colors.h"

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellPuck::LLSFRefBoxShellPuck(int begin_y, int begin_x)
  : NCursesPanel(1, 10, begin_y, begin_x),
    id_(0), state_(llsf_msgs::S0), at_machine_(false)
{
}


LLSFRefBoxShellPuck::~LLSFRefBoxShellPuck()
{
}


void
LLSFRefBoxShellPuck::update(unsigned int id, llsf_msgs::PuckState state, bool at_machine)
{
  id_ = id;
  state_ = state;
  at_machine_ = at_machine;
}


void
LLSFRefBoxShellPuck::reset()
{
  id_ = 0;
  state_ = llsf_msgs::S0;
  at_machine_ = false;
  refresh();
}

int
LLSFRefBoxShellPuck::refresh()
{
  standend();
  erase();
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));

  if (id_ != 0) {
    attron(A_BOLD);
    printw("%6u", id_);
    attroff(A_BOLD);
  }
  attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  if (state_ != llsf_msgs::S0) {
    attron(A_BOLD);
  }
  if (state_ == llsf_msgs::CONSUMED) {
    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
  } else if (state_ == llsf_msgs::S1) {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN));
  } else if (state_ == llsf_msgs::S2) {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_YELLOW));
  } else if (state_ == llsf_msgs::P1 || state_ == llsf_msgs::P2 || state_ == llsf_msgs::P3) {
    attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));
  }

  printw(0, 7, "%2s", (id_ == 0) ? "" : llsf_msgs::PuckState_Name(state_).substr(0,2).c_str());
  standend();
  if (at_machine_) {
    addch(0, 9, '*');
  }

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
