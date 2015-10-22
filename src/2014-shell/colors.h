
/***************************************************************************
 *  colors.h - LLSF RefBox shell
 *
 *  Created: Wed Mar 06 14:12:13 2013
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

#ifndef __LLSF_REFBOX_SHELL_COLORS_H_
#define __LLSF_REFBOX_SHELL_COLORS_H_

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define COLOR_DEFAULT           0
#define COLOR_BLACK_ON_WHITE    1
#define COLOR_WHITE_ON_RED      2
#define COLOR_WHITE_ON_YELLOW   3
#define COLOR_WHITE_ON_GREEN    4
#define COLOR_GREEN_ON_BACK     5
#define COLOR_RED_ON_BACK       6
#define COLOR_YELLOW_ON_BACK    7
#define COLOR_BLACK_ON_BACK     8
#define COLOR_WHITE_ON_BACK     9
#define COLOR_CYAN_ON_BACK     10
#define COLOR_MAGENTA_ON_BACK  11
#define COLOR_WHITE_ON_CYAN    12
#define COLOR_WHITE_ON_MAGENTA 13

extern void init_colors();

} // end of namespace llsfrb_shell

#endif
