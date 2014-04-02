
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

#include "team.h"

#include <cstddef>
#include <cursesm.h>
#include <string>
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <msgs/MachineInfo.pb.h>
#include <msgs/PuckInfo.pb.h>
#include <msgs/GameInfo.pb.h>
#include <msgs/RobotInfo.pb.h>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class PassiveItem : public NCursesMenuItem
{
 public:
  PassiveItem(const std::string &text) : NCursesMenuItem(text.c_str())
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
  Menu(int nlines, int ncols, int begin_y, int begin_x);
  virtual NCursesMenuItem* operator()(void);

 private:
  void start_keyboard();
  void handle_keyboard(const boost::system::error_code& error);

 private:
  boost::asio::io_service io_service_;
  boost::asio::posix::stream_descriptor stdin_;
};


class GenericItemsMenu : public Menu
{
 public:
  GenericItemsMenu(NCursesWindow *parent, int n_items, NCursesMenuItem **items);

 private:
  virtual void On_Menu_Init();
  int max_cols(int n_items, NCursesMenuItem **items);

 private:
  NCursesWindow *parent_;
};


class MachineWithPuckMenu : public Menu
{
 public:
  MachineWithPuckMenu(NCursesWindow *parent, Team team,
		      std::shared_ptr<llsf_msgs::MachineInfo> minfo);

  void get_machine_puck(std::string &machine_name, unsigned int &puck_id);
  operator bool() const { return valid_item_; }

 private:
  virtual void On_Menu_Init();
  int det_lines(Team team, std::shared_ptr<llsf_msgs::MachineInfo> &minfo);
  void puck_selected(std::string machine, unsigned int puck_id);

 private:
  bool valid_item_;
  std::string s_cancel_;
  std::string machine_name_;
  unsigned int puck_id_;
  typedef std::tuple<std::string, std::string, unsigned int, llsf_msgs::PuckState> ItemTuple;
  std::vector<ItemTuple> items_;
};


class RobotMaintenanceMenu : public Menu
{
 public:
  RobotMaintenanceMenu(NCursesWindow *parent, Team team,
		       std::shared_ptr<llsf_msgs::RobotInfo> minfo);

  void get_robot(unsigned int &number, bool &maintenance);
  operator bool() const { return valid_item_; }

 private:
  virtual void On_Menu_Init();
  int det_lines(Team team, std::shared_ptr<llsf_msgs::RobotInfo> &rinfo);
  int det_cols(std::shared_ptr<llsf_msgs::RobotInfo> &rinfo);
  void robot_selected(unsigned int number, bool maintenance);

 private:
  bool valid_item_;
  unsigned int robot_number_;
  bool robot_maintenance_;
  std::string s_cancel_;
  typedef std::tuple<std::string, unsigned int, bool> ItemTuple;
  std::vector<ItemTuple> items_;
};


class MachineThatCanTakePuckMenu : public Menu
{
 public:
  MachineThatCanTakePuckMenu(NCursesWindow *parent, Team team,
			     std::shared_ptr<llsf_msgs::MachineInfo> minfo);


  const llsf_msgs::Machine &  machine();
  operator bool() const;

 private:
  virtual void On_Menu_Init();
  int det_lines(Team team, std::shared_ptr<llsf_msgs::MachineInfo> &minfo);
  void machine_selected(int i);

 private:
  std::shared_ptr<llsf_msgs::MachineInfo> minfo_;
  bool machine_selected_;
  int machine_idx_;
  std::string s_cancel_;
  typedef std::tuple<std::string, std::string, int> ItemTuple;
  std::vector<ItemTuple> items_;
};


class PuckForMachineMenu : public Menu
{
 public:
  PuckForMachineMenu(NCursesWindow *parent, Team team,
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
				const llsf_msgs::Machine &machine, Team team);
  int det_lines(std::shared_ptr<llsf_msgs::PuckInfo> &pinfo,
		std::shared_ptr<llsf_msgs::MachineInfo> &minfo,
		const llsf_msgs::Machine &machine, Team team);

 private:
  std::shared_ptr<llsf_msgs::PuckInfo> pinfo_;
  bool puck_selected_;
  int puck_idx_;
  std::string s_cancel_;
  std::vector<std::tuple<std::string, int>> items_;
};


class MachinePlacingMenu : public Menu
{
 public:
  MachinePlacingMenu(NCursesWindow *parent, std::string machine, std::string puck,
		     bool can_be_placed_under_rfid, bool can_be_loaded_with);

  bool place_under_rfid();

  operator bool() const;

 private:
  void item_selected(bool under_rfid);
  virtual void On_Menu_Init();
  int det_lines(bool can_be_placed_under_rfid, bool can_be_loaded_with);

 private:
  bool valid_selected_;
  bool place_under_rfid_;
  std::string s_cancel_, s_under_rfid_, s_loaded_with_;
};


class TeamSelectMenu : public Menu
{
 public:
  TeamSelectMenu(NCursesWindow *parent, std::shared_ptr<llsf_msgs::GameInfo> gameinfo);

  std::string get_team_name();
  operator bool() const { return valid_item_; }

 private:
  virtual void On_Menu_Init();
  int det_lines(std::shared_ptr<llsf_msgs::GameInfo> &gameinfo);
  void team_selected(std::string team_name);

 private:
  bool valid_item_;
  std::string s_cancel_;
  std::string team_name_;
  std::vector<std::string> items_;
};


} // end of namespace llsfrb

#endif
