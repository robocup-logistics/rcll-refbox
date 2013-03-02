
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

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellPuck::LLSFRefBoxShellPuck(int begin_y, int begin_x)
  : NCursesPanel(1, 9, begin_y, begin_x),
    id_(0), state_(llsf_msgs::S0)
{
}


LLSFRefBoxShellPuck::~LLSFRefBoxShellPuck()
{
}


void
LLSFRefBoxShellPuck::update(unsigned int id, llsf_msgs::PuckState state)
{
  id_ = id;
  state_ = state;
}


void
LLSFRefBoxShellPuck::reset()
{
  id_ = 0;
  state_ = llsf_msgs::S0;
}

int
LLSFRefBoxShellPuck::refresh()
{
  standend();
  erase();
  bkgd(' '|COLOR_PAIR(0));

  if (id_ != 0) {
    attron(A_BOLD);
    printw("%6u", id_);
    attroff(A_BOLD);
  }
  attron(' '|COLOR_PAIR(1)|A_BOLD);
  printw(0, 7, "%2s", (id_ == 0) ? "" : llsf_msgs::PuckState_Name(state_).substr(0,2).c_str());
  standend();

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
