
/***************************************************************************
 *  machine.h - LLSF RefBox shell - Machine
 *
 *  Created: Thu Feb 21 18:02:53 2013
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

#ifndef __LLSF_REFBOX_SHELL_MACHINE_H_
#define __LLSF_REFBOX_SHELL_MACHINE_H_

#include <cstddef>
#include <cursesp.h>

#include <msgs/MachineInfo.pb.h>

#include <vector>
#include <map>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


class LLSFRefBoxShellMachine : public NCursesPanel
{
 public:
  LLSFRefBoxShellMachine(std::string name, std::string type,
			 int begin_y, int begin_x, bool visible);
  ~LLSFRefBoxShellMachine();

  void set_type(std::string type);
  void set_lights(std::map<llsf_msgs::LightColor, llsf_msgs::LightState> &lights);
  void set_inputs(std::vector<llsf_msgs::PuckState> &inputs);
  void set_loaded_with(std::vector<llsf_msgs::PuckState> &loaded_with);
  void set_correctly_reported(bool has_field, bool correctly_reported = false);
  void set_puck_under_rfid(bool has_puck, llsf_msgs::PuckState puck_state = llsf_msgs::S0);
  void flip_blink_states();

  void set_visible(bool visible);

  void reset();

  int refresh();

 private:
  bool          visible_;
  std::string   name_;
  std::string   type_;
  std::vector<llsf_msgs::PuckState> inputs_;
  std::vector<llsf_msgs::PuckState> loaded_with_;
  bool                              puck_under_rfid_;
  llsf_msgs::PuckState              puck_under_rfid_state_;
  bool                              has_correctly_reported_field_;
  bool                              correctly_reported_;
  std::map<llsf_msgs::LightColor, llsf_msgs::LightState> lights_;
  std::map<llsf_msgs::LightColor, bool> blink_state_;
};


} // end of namespace llsfrb

#endif
