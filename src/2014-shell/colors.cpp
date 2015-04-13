
/***************************************************************************
 *  colors.cpp - LLSF RefBox shell
 *
 *  Created: Wed Mar 06 14:25:31 2013
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

#include "colors.h"
#include <curses.h>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

void
init_colors()
{
  short default_fore, default_back;
  pair_content(0, &default_fore, &default_back);
  //chtype default_back = getbkgd(stdscr);

  init_pair(COLOR_RED_ON_BACK, COLOR_RED, default_back);
  init_pair(COLOR_YELLOW_ON_BACK, COLOR_YELLOW, default_back);
  init_pair(COLOR_BLACK_ON_BACK, default_fore, default_back);
  init_pair(COLOR_WHITE_ON_BACK, COLOR_WHITE, default_back);
  init_pair(COLOR_WHITE_ON_RED, COLOR_WHITE, COLOR_RED);
  init_pair(COLOR_GREEN_ON_BACK, COLOR_GREEN, default_back);
  init_pair(COLOR_BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
  init_pair(COLOR_WHITE_ON_RED, COLOR_WHITE, COLOR_RED);
  init_pair(COLOR_WHITE_ON_YELLOW, COLOR_WHITE, COLOR_YELLOW);
  init_pair(COLOR_WHITE_ON_GREEN, COLOR_WHITE, COLOR_GREEN);
  init_pair(COLOR_CYAN_ON_BACK, COLOR_CYAN, default_back);
  init_pair(COLOR_MAGENTA_ON_BACK, COLOR_MAGENTA, default_back);
  init_pair(COLOR_WHITE_ON_CYAN, COLOR_WHITE, COLOR_CYAN);
  init_pair(COLOR_WHITE_ON_MAGENTA, COLOR_WHITE, COLOR_MAGENTA);

}

} // end of namespace llsfrb_shell
