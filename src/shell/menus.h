
/***************************************************************************
 *  menus.h - LLSF RefBox shell menus
 *
 *  Created: Sun Mar 03 00:28:50 2013
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

#ifndef __LLSF_REFBOX_SHELL_MENUS_H_
#define __LLSF_REFBOX_SHELL_MENUS_H_

#include <cursesm.h>
#include <string>
#include <boost/signals2.hpp>
#include <msgs/MachineInfo.pb.h>
#include <msgs/PuckInfo.pb.h>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class PassiveItem : public NCursesMenuItem
{
 public:
  PassiveItem(const char* text) : NCursesMenuItem(text)
  { options_off(O_SELECTABLE); }
};


class SignalItem : public NCursesMenuItem
{
public:
  SignalItem(const std::string &s) : NCursesMenuItem(s.c_str())
  {}

  boost::signals2::signal<void ()> & signal() { return signal_; }

  bool action() { signal_(); return TRUE; }

  private:
    boost::signals2::signal<void ()> signal_;
};

class Menu : public NCursesMenu
{
 public:
  Menu(NCursesWindow *parent, int n_items, NCursesMenuItem **items);

 private:
  virtual void On_Menu_Init();
  int max_cols(int n_items, NCursesMenuItem **items);

 private:
  NCursesWindow *parent_;
};


class MachineWithPuckMenu : public NCursesMenu
{
 public:
  MachineWithPuckMenu(NCursesWindow *parent, std::shared_ptr<llsf_msgs::MachineInfo> minfo);

  void get_machine_puck(std::string &machine_name, unsigned int &puck_id);
  operator bool() const { return valid_item_; }

 private:
  virtual void On_Menu_Init();
  int det_lines(std::shared_ptr<llsf_msgs::MachineInfo> &minfo);
  void puck_selected(std::string machine, unsigned int puck_id);

 private:
  bool valid_item_;
  std::string s_cancel_;
  std::string machine_name_;
  unsigned int puck_id_;
  std::vector<std::tuple<std::string, std::string, unsigned int>> items_;
};


class MachineThatCanTakePuckMenu : public NCursesMenu
{
 public:
  MachineThatCanTakePuckMenu(NCursesWindow *parent,
			     std::shared_ptr<llsf_msgs::MachineInfo> minfo);


  const llsf_msgs::Machine &  machine();
  operator bool() const;

 private:
  virtual void On_Menu_Init();
  int det_lines(std::shared_ptr<llsf_msgs::MachineInfo> &minfo);
  void machine_selected(int i);

 private:
  std::shared_ptr<llsf_msgs::MachineInfo> minfo_;
  bool machine_selected_;
  int machine_idx_;
  std::string s_cancel_;
  std::vector<std::tuple<std::string, int>> items_;
};


class PuckForMachineMenu : public NCursesMenu
{
 public:
  PuckForMachineMenu(NCursesWindow *parent,
		     std::shared_ptr<llsf_msgs::PuckInfo> pinfo,
		     std::shared_ptr<llsf_msgs::MachineInfo> minfo,
		     const llsf_msgs::Machine &machine);


  const llsf_msgs::Puck & puck();
  operator bool() const;

 private:
  void puck_selected(int i);
  virtual void On_Menu_Init();
  std::list<int> relevant_pucks(std::shared_ptr<llsf_msgs::PuckInfo> &pinfo,
				std::shared_ptr<llsf_msgs::MachineInfo> &minfo,
				const llsf_msgs::Machine &machine);
  int det_lines(std::shared_ptr<llsf_msgs::PuckInfo> &pinfo,
		std::shared_ptr<llsf_msgs::MachineInfo> &minfo,
		const llsf_msgs::Machine &machine);

 private:
  std::shared_ptr<llsf_msgs::PuckInfo> pinfo_;
  bool puck_selected_;
  int puck_idx_;
  std::string s_cancel_;
  std::vector<std::tuple<std::string, int>> items_;
};


class MachinePlacingMenu : public NCursesMenu
{
 public:
  MachinePlacingMenu(NCursesWindow *parent, std::string machine, std::string puck);

  bool place_under_rfid();

  operator bool() const;

 private:
  void item_selected(bool under_rfid);
  virtual void On_Menu_Init();

 private:
  bool valid_selected_;
  bool place_under_rfid_;
  std::string s_cancel_, s_under_rfid_, s_loaded_with_;
};


} // end of namespace llsfrb

#endif
