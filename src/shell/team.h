
/***************************************************************************
 *  team.h - LLSF RefBox team identifier
 *
 *  Created: Thu Mar 27 21:44:49 2014
 *  Copyright  2014  Tim Niemueller [www.niemueller.de]
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

#ifndef __LLSF_REFBOX_SHELL_TEAM_H_
#define __LLSF_REFBOX_SHELL_TEAM_H_

#include <msgs/Team.pb.h>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

typedef enum {
  CYAN = 0,
  MAGENTA = 1
} Team;

#define TEAM_NUM 2
#define TEAM_NUM_MACHINES 16

static const char * TEAM_MACHINES[TEAM_NUM][TEAM_NUM_MACHINES] __attribute((__used__)) =
  { /* CYAN */    { "M1", "M2", "M3", "M4", "M5", "M6",
		    "M7", "M8", "M9", "M10", "M11", "M12", "D1", "D2", "D3", "R1" },
    /* MAGENTA */ { "M13", "M14", "M15", "M16", "M17", "M18", "M19",
		    "M20", "M21", "M22", "M23", "M24", "D4", "D5", "D6", "R2"}};

inline bool
operator==(const llsf_msgs::Team &t1, const Team &t2)
{
  return ((t1 == llsf_msgs::CYAN) && (t2 == CYAN)) ||
         ((t1 == llsf_msgs::MAGENTA) && (t2 == MAGENTA));
}

inline bool
operator!=(const llsf_msgs::Team &t1, const Team &t2)
{
  return ((t1 == llsf_msgs::CYAN) && (t2 != CYAN)) ||
         ((t1 == llsf_msgs::MAGENTA) && (t2 != MAGENTA));
}

} // end of namespace llsfrb_shell

#endif
