
/***************************************************************************
 *  robot.h - LLSF RefBox shell - Robot
 *
 *  Created: Thu Feb 21 22:28:42 2013
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

#ifndef __LLSF_REFBOX_SHELL_ROBOT_H_
#define __LLSF_REFBOX_SHELL_ROBOT_H_

#include <cstddef>
#include <cursesp.h>

#include <string>
#include <boost/date_time.hpp>
#include <msgs/RobotInfo.pb.h>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


class LLSFRefBoxShellRobot : public NCursesPanel
{
 public:
  LLSFRefBoxShellRobot(int begin_y, int begin_x);

  void update(unsigned int number, std::string name, std::string team, std::string host,
	      llsf_msgs::RobotState state, float maintenance_time_remaining,
	      unsigned int maintenance_cycles);
  void set_last_seen(boost::posix_time::ptime &last_seen);
  void reset();

  int refresh();

  bool is_busy() const { return (name_ != ""); }
  const std::string &  name() const { return name_; }

 private:
  unsigned int  number_;
  std::string   name_;
  std::string   team_;
  std::string   host_;
  llsf_msgs::RobotState state_;
  float maintenance_time_remaining_;
  unsigned int maintenance_cycles_;
  boost::posix_time::ptime last_seen_;
};


} // end of namespace llsfrb

#endif
