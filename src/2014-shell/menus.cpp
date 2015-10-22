
/***************************************************************************
 *  menus.cpp - LLSF RefBox shell menus
 *
 *  Created: Sun Mar 03 00:29:20 2013
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

#include "menus.h"
#include "colors.h"
#include <boost/format.hpp>

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

static const int CMD_QUIT   = MAX_COMMAND + 1;
static const int CMD_ACTION = MAX_COMMAND + 2;

Menu::Menu(int nlines, int ncols, int begin_y, int begin_x)
: NCursesMenu(nlines, ncols, begin_y, begin_x),
  stdin_(io_service_, dup(STDIN_FILENO))
{}

NCursesMenuItem*
Menu::operator()(void)
{
  set_current(* (*this)[0]);

  post();
  show();
  refresh();

  nodelay(true);
  start_keyboard();
  io_service_.run();
  io_service_.reset();

  unpost();
  hide();
  refresh();
  if (options() & O_ONEVALUE)
    return current_item();
  else
    return NULL;
}

void
Menu::start_keyboard()
{
  stdin_.async_read_some(boost::asio::null_buffers(),
			 boost::bind(&Menu::handle_keyboard, this,
				     boost::asio::placeholders::error));
}

void
Menu::handle_keyboard(const boost::system::error_code& error)
{
  if (! error) {
    int drvCmnd;
    int err;
    int c;
    bool b_action = FALSE;
    while ((c=getKey()) != ERR) {
      if ((drvCmnd = virtualize(c)) == CMD_QUIT)  break;
      switch((err=driver(drvCmnd))) {
      case E_REQUEST_DENIED:
	On_Request_Denied(c);
	break;
      case E_NOT_SELECTABLE:
	On_Not_Selectable(c);
	break;
      case E_UNKNOWN_COMMAND:
	if (drvCmnd == CMD_ACTION) {
	  if (options() & O_ONEVALUE) {
	    NCursesMenuItem* itm = current_item();
	    assert(itm != 0);
	    if (itm->options() & O_SELECTABLE)
	    {
	      b_action = itm->action();
	      refresh();
	    }
	    else
	      On_Not_Selectable(c);
	  }
	  else {
	    int n = count();
	    for(int i=0; i<n; i++) {
	      NCursesMenuItem* itm = (*this)[i];
	      if (itm->value()) {
		b_action |= itm->action();
		refresh();
	      }
	    }
	  }
	} else
	  On_Unknown_Command(c);
	break;
      case E_NO_MATCH:
	On_No_Match(c);
	break;
      case E_OK:
	break;
      default:
	OnError(err);
      }
    }
    
    if (!b_action) {
      start_keyboard();
    }
  }
}


GenericItemsMenu::GenericItemsMenu(NCursesWindow *parent, int n_items, NCursesMenuItem **items)
  : Menu(n_items + 2, max_cols(n_items, items) + 2,
	 (parent->lines() - max_cols(n_items, items))/2,
	 (parent->cols() - max_cols(n_items, items))/2),
    parent_(parent)
{
  set_mark("");
  InitMenu(items, true, true);
}

void
GenericItemsMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  //subWindow().bkgd(parent_->getbkgd());
  refresh();
}

int
GenericItemsMenu::max_cols(int n_items, NCursesMenuItem **items)
{
  int rv = 0;
  for (int i = 0; i < n_items; ++i) {
    rv = std::max(rv, (int)strlen(items[i]->name()));
  }
  return rv;
}

MachineWithPuckMenu::MachineWithPuckMenu(NCursesWindow *parent, Team team,
					 std::shared_ptr<llsf_msgs::MachineInfo> minfo)
  : Menu(det_lines(team, minfo) + 1 + 2, 12 + 2,
	 (parent->lines() - (det_lines(team, minfo) + 1))/2,
	 (parent->cols() - 12)/2)
{
  valid_item_ = false;
  int n_items = det_lines(team, minfo);
  items_.resize(n_items);
  int ni = 0;
  NCursesMenuItem **mitems = new NCursesMenuItem*[2 + n_items];
  for (int i = 0; i < minfo->machines_size(); ++i) {
    const llsf_msgs::Machine &m = minfo->machines(i);
    if (m.team_color() != team) continue;
    if (m.has_puck_under_rfid()) {
      items_[ni++] =
	std::make_tuple("* " + m.name() + " " +
			llsf_msgs::PuckState_Name(m.puck_under_rfid().state()).substr(0,2),
			m.name(), m.puck_under_rfid().id(), m.puck_under_rfid().state());
    }
    for (int l = 0; l < m.loaded_with_size(); ++l) {
      items_[ni++] =
	std::make_tuple("  " + m.name() + " " +
			llsf_msgs::PuckState_Name(m.loaded_with(l).state()).substr(0,2),
			m.name(), m.loaded_with(l).id(), m.loaded_with(l).state());
    }
  }

  std::sort(items_.begin(), items_.end(),
	    [](const ItemTuple &i1, const ItemTuple &i2) -> bool
	    {
	      const std::string &s1 = std::get<1>(i1);
	      const std::string &s2 = std::get<1>(i2);
	      std::string f1 = s1.substr(0,1);
	      std::string f2 = s2.substr(0,1);
	      bool star1 = (std::get<0>(i1)[0] == '*');
	      bool star2 = (std::get<0>(i2)[0] == '*');
	      int n1 = 0, n2 = 0;
	      try {
		n1 = std::stoi(s1.substr(1));
		n2 = std::stoi(s2.substr(1));
	      } catch (std::invalid_argument &e) {} // ignored
	      return 
		((f1 == f2) &&
		 ((n1 < n2) || ((n1 == n2) && (star1 && ! star2)) ||
		  (std::get<3>(i1) < std::get<3>(i2)))) ||
		((f1 == "M") && (f2 != "M")) ||
		((f1 == "D") && (f2.find_first_of("MD") == std::string::npos)) ||
		((f1 == "T") && (f2.find_first_of("MDT") == std::string::npos)) ||
		((f1 == "R") && (f2.find_first_of("MDTR") == std::string::npos));
	    });

  for (int i = 0; i < ni; ++i) {
    SignalItem *item = new SignalItem(std::get<0>(items_[i]));
    item->signal().connect(boost::bind(&MachineWithPuckMenu::puck_selected, this,
				       std::get<1>(items_[i]), std::get<2>(items_[i])));
    mitems[i] = item;
  }
  s_cancel_ = "** CANCEL **";
  mitems[ni] = new SignalItem(s_cancel_);
  mitems[ni+1] = new NCursesMenuItem();

  set_mark("");
  set_format(ni+1, 1);
  InitMenu(mitems, true, true);
  frame("Puck");
}

void
MachineWithPuckMenu::puck_selected(std::string machine, unsigned int puck_id)
{
  valid_item_ = true;
  machine_name_ = machine;
  puck_id_ = puck_id;
}

void
MachineWithPuckMenu::get_machine_puck(std::string &machine_name, unsigned int &puck_id)
{
  machine_name = machine_name_;
  puck_id = puck_id_;
}

void
MachineWithPuckMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  //subWindow().bkgd(parent_->getbkgd());
  refresh();
}

int
MachineWithPuckMenu::det_lines(Team team, std::shared_ptr<llsf_msgs::MachineInfo> &minfo)
{
  int rv = 0;
  for (int i = 0; i < minfo->machines_size(); ++i) {
    const llsf_msgs::Machine &m = minfo->machines(i);
    if (m.team_color() != team) continue;
    if (m.has_puck_under_rfid()) {
      rv += 1;
    }
    rv += m.loaded_with_size();
  }
  return rv;
}


MachineThatCanTakePuckMenu::MachineThatCanTakePuckMenu(
  NCursesWindow *parent, Team team,
  std::shared_ptr<llsf_msgs::MachineInfo> minfo)
  : Menu(det_lines(team, minfo) + 1 + 2, 24 + 2,
	 (parent->lines() - (det_lines(team, minfo) + 1))/2,
	 (parent->cols() - 26)/2),
    minfo_(minfo)
{
  machine_selected_ = false;
  int n_items = det_lines(team, minfo);
  items_.resize(n_items);
  int ni = 0;
  NCursesMenuItem **mitems = new NCursesMenuItem*[2 + n_items];
  for (int i = 0; i < minfo->machines_size(); ++i) {
    const llsf_msgs::Machine &m = minfo->machines(i);
    if (m.team_color() != team) continue;
    if (! m.has_puck_under_rfid() || (m.inputs_size() - m.loaded_with_size() > 0)) {
      std::string s = boost::str(boost::format("%-3s|%s") % m.name() % m.type());
      items_[ni++] = std::make_tuple(s, m.name(), i);
    }
  }
  std::sort(items_.begin(), items_.end(),
	    [](const ItemTuple &i1, const ItemTuple &i2) -> bool
	    {
	      const std::string &s1 = std::get<1>(i1);
	      const std::string &s2 = std::get<1>(i2);
	      std::string f1 = s1.substr(0,1);
	      std::string f2 = s2.substr(0,1);
	      int n1 = 0, n2 = 0;
	      try {
		n1 = std::stoi(s1.substr(1));
		n2 = std::stoi(s2.substr(1));
	      } catch (std::invalid_argument &e) {} // ignored
	      return 
		((f1 == f2) && (n1 < n2)) ||
		((f1 == "M") && (f2 != "M")) ||
		((f1 == "D") && (f2.find_first_of("MD") == std::string::npos)) ||
		((f1 == "T") && (f2.find_first_of("MDT") == std::string::npos)) ||
		((f1 == "R") && (f2.find_first_of("MDTR") == std::string::npos));
	    });

  for (int i = 0; i < ni; ++i) {
    SignalItem *item = new SignalItem(std::get<0>(items_[i]));
    item->signal().connect(boost::bind(&MachineThatCanTakePuckMenu::machine_selected, this,
				       std::get<2>(items_[i])));
    mitems[i] = item;
  }
  s_cancel_ = "** CANCEL **";
  mitems[ni] = new SignalItem(s_cancel_);
  mitems[ni+1] = new NCursesMenuItem();

  set_mark("");
  set_format(ni+1, 1);
  InitMenu(mitems, true, true);
  frame("Machine");
}

void
MachineThatCanTakePuckMenu::machine_selected(int i)
{
  machine_selected_ = true;
  machine_idx_ = i;
}

const llsf_msgs::Machine &
MachineThatCanTakePuckMenu::machine()
{
  return minfo_->machines(machine_idx_);
}

void
MachineThatCanTakePuckMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  //subWindow().bkgd(parent_->getbkgd());

  for (size_t i = 0; i < items_.size(); ++i) {
    const llsf_msgs::Machine &m = minfo_->machines(std::get<2>(items_[i]));
    if (m.puck_under_rfid().id() != 0) {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE)|A_BOLD);
      addstr(i+1, 13,
	     llsf_msgs::PuckState_Name(m.puck_under_rfid().state()).substr(0,2).c_str());
      attroff(A_BOLD);
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
      addstr(i+1, 13, "  ");
    }

    int puck_x = 16;
    for (int j = 0; j < m.inputs_size(); ++j) {
      llsf_msgs::PuckState ps = m.inputs(j);
      bool puck_loaded = false;
      for (int k = 0; k < m.loaded_with_size(); ++k) {
	if (m.loaded_with(k).state() == ps) {
	  puck_loaded = true;
	  break;
	}
      }

      if (puck_loaded) {
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED)|A_BOLD);
      } else {
	attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
      }

      addstr(i+1, puck_x, llsf_msgs::PuckState_Name(ps).substr(0,2).c_str());
      attroff(A_BOLD);
      puck_x += 3;
    }
  }

  refresh();
}

int
MachineThatCanTakePuckMenu::det_lines(Team team,
				      std::shared_ptr<llsf_msgs::MachineInfo> &minfo)
{
  int rv = 0;
  for (int i = 0; i < minfo->machines_size(); ++i) {
    const llsf_msgs::Machine &m = minfo->machines(i);
    if (m.team_color() != team) continue;
    if (! m.has_puck_under_rfid() || (m.inputs_size() - m.loaded_with_size() > 0)) {
      rv += 1;
    }
  }
  return rv;
}

MachineThatCanTakePuckMenu::operator bool() const
{
  return machine_selected_;
}


PuckForMachineMenu::PuckForMachineMenu(NCursesWindow *parent, Team team,
				       std::shared_ptr<llsf_msgs::PuckInfo> pinfo,
				       std::shared_ptr<llsf_msgs::MachineInfo> minfo,
				       const llsf_msgs::Machine &machine)
  : Menu(det_lines(pinfo, minfo, machine, team) + 1 + 2, 14 + 2,
	 (parent->lines() - (det_lines(pinfo, minfo, machine, team) + 1))/2,
	 (parent->cols() - 14)/2),
    pinfo_(pinfo)
{
  puck_selected_ = false;
  std::list<int> rel_pucks = relevant_pucks(pinfo, minfo, machine, team);
  items_.resize(rel_pucks.size() + 1);
  int ni = 0;
  NCursesMenuItem **mitems = new NCursesMenuItem*[2 + rel_pucks.size()];
  std::list<int>::iterator i;
  for (i = rel_pucks.begin(); i != rel_pucks.end(); ++i) {
    const llsf_msgs::Puck &p = pinfo->pucks(*i);
    items_[ni++] = std::make_tuple(llsf_msgs::PuckState_Name(p.state()).substr(0,2) +
				   " (" + std::to_string(p.id()) + ")", *i);
  }
  //std::sort(items_.begin(), items_.end());

  for (int i = 0; i < ni; ++i) {
    SignalItem *item = new SignalItem(std::get<0>(items_[i]));
    item->signal().connect(boost::bind(&PuckForMachineMenu::puck_selected, this,
				       std::get<1>(items_[i])));
    mitems[i] = item;
  }
  s_cancel_ = "** CANCEL **";
  mitems[ni] = new SignalItem(s_cancel_);
  mitems[ni+1] = new NCursesMenuItem();

  set_format(ni+1, 1);
  set_mark("");
  InitMenu(mitems, true, true);
  frame("Puck");
}

void
PuckForMachineMenu::puck_selected(int i)
{
  puck_selected_ = true;
  puck_idx_ = i;
}

const llsf_msgs::Puck &
PuckForMachineMenu::puck()
{
  return pinfo_->pucks(puck_idx_);
}

void
PuckForMachineMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  //subWindow().bkgd(parent_->getbkgd());
  refresh();
}

std::list<int>
PuckForMachineMenu::relevant_pucks(std::shared_ptr<llsf_msgs::PuckInfo> &pinfo,
				   std::shared_ptr<llsf_msgs::MachineInfo> &minfo,
				   const llsf_msgs::Machine &machine, Team team)
{
  std::list<int> rv;
  for (int i = 0; i < pinfo->pucks_size(); ++i) {
    const llsf_msgs::Puck &p = pinfo->pucks(i);
    if (p.team_color() != team) continue;
    rv.push_back(i);
  }

  // filter out all pucks which are already bound at a machine
  rv.remove_if([&minfo,&pinfo](int p)
	       {
		 for (int i = 0; i < minfo->machines_size(); ++i) {
		   const llsf_msgs::Machine &m = minfo->machines(i);
		   if (m.has_puck_under_rfid() && m.puck_under_rfid().id() == pinfo->pucks(p).id()) {
		     return true;
		   }
		   for (int j = 0; j < m.loaded_with_size(); ++j) {
		     if (m.loaded_with(j).id() == pinfo->pucks(p).id()) {
		       return true;
		     }
		   }
		 }
		 return false;
	       });

  if (machine.inputs_size() > 0) {
    // filter out all pucks which are not avalid input for the current machine
    rv.remove_if([&pinfo,&machine](int p)
		 {
		   for (int i = 0; i < machine.inputs_size(); ++i) {
		     if (pinfo->pucks(p).state() == machine.inputs(i)) return false;
		   }
		   return true;
		 });
  }

  // filter out all pucks in a state already placed at the machine
  rv.remove_if([&pinfo,&machine](int p)
	       {
		 if (machine.has_puck_under_rfid() &&
		     machine.puck_under_rfid().state() == pinfo->pucks(p).state())
		 {
		   return true;
		 }
	
		 for (int i = 0; i < machine.loaded_with_size(); ++i) {
		   if (pinfo->pucks(p).state() == machine.loaded_with(i).state()) return true;
		 }
		 return false;
	       });

  return rv;    
}

int
PuckForMachineMenu::det_lines(std::shared_ptr<llsf_msgs::PuckInfo> &pinfo,
			      std::shared_ptr<llsf_msgs::MachineInfo> &minfo,
			      const llsf_msgs::Machine &machine, Team team)
{
  return relevant_pucks(pinfo, minfo, machine, team).size();
}

PuckForMachineMenu::operator bool() const
{
  return puck_selected_;
}


MachinePlacingMenu::MachinePlacingMenu(NCursesWindow *parent,
				       std::string machine, std::string puck,
				       bool can_be_placed_under_rfid, bool can_be_loaded_with)
  : Menu(det_lines(can_be_placed_under_rfid, can_be_loaded_with), 30,
	 (parent->lines() - 5)/2, (parent->cols() - 30)/2)
{
  valid_selected_ = false;
  place_under_rfid_ = false;
  s_under_rfid_ = "Place " + puck.substr(0,2) + " under RFID of " + machine;
  s_loaded_with_ = "Load " + machine + " with " + puck.substr(0,2);
  s_cancel_ = "** CANCEL **";
  NCursesMenuItem **mitems = new NCursesMenuItem*[4];

  int idx = 0;

  if (can_be_placed_under_rfid) {
    SignalItem *item = new SignalItem(s_under_rfid_);
    item->signal().connect(boost::bind(&MachinePlacingMenu::item_selected, this, true));
    mitems[idx++] = item;
  }

  if (can_be_loaded_with) {
    SignalItem *item = new SignalItem(s_loaded_with_);
    item->signal().connect(boost::bind(&MachinePlacingMenu::item_selected, this, false));
    mitems[idx++] = item;
  }
  mitems[idx++] = new SignalItem(s_cancel_);
  mitems[idx++] = new NCursesMenuItem();
    

  set_mark("");
  set_format(idx-1, 1);
  InitMenu(mitems, true, true);
  frame("Placing");
}

int
MachinePlacingMenu::det_lines(bool can_be_placed_under_rfid, bool can_be_loaded_with)
{
  int rv = 3;
  if (can_be_placed_under_rfid) rv += 1;
  if (can_be_loaded_with) rv += 1;
  return rv;
}

void
MachinePlacingMenu::item_selected(bool under_rfid)
{
  valid_selected_ = true;
  place_under_rfid_ = under_rfid;
}

bool
MachinePlacingMenu::place_under_rfid()
{
  return place_under_rfid_;
}

void
MachinePlacingMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  //subWindow().bkgd(parent_->getbkgd());
  refresh();
}

MachinePlacingMenu::operator bool() const
{
  return valid_selected_;
}


TeamSelectMenu::TeamSelectMenu(NCursesWindow *parent,
			       std::shared_ptr<llsf_msgs::GameInfo> gameinfo)
  : Menu(det_lines(gameinfo) + 1 + 2, 20 + 2,
	 (parent->lines() - (det_lines(gameinfo) + 1))/2,
	 (parent->cols() - 20)/2)
{
  valid_item_ = false;
  int n_items = det_lines(gameinfo);
  items_.resize(n_items);
  int ni = 0;
  NCursesMenuItem **mitems = new NCursesMenuItem*[2 + n_items];
  for (int i = 0; i < gameinfo->known_teams_size(); ++i) {
    items_[ni++] = gameinfo->known_teams(i);
  }

  std::sort(items_.begin(), items_.end());

  for (int i = 0; i < ni; ++i) {
    SignalItem *item = new SignalItem(items_[i]);
    item->signal().connect(boost::bind(&TeamSelectMenu::team_selected, this, items_[i]));
    mitems[i] = item;
  }
  s_cancel_ = "** CANCEL **";
  mitems[ni] = new SignalItem(s_cancel_);
  mitems[ni+1] = new NCursesMenuItem();

  set_mark("");
  set_format(ni+1, 1);
  InitMenu(mitems, true, true);
  frame("Team");
}

void
TeamSelectMenu::team_selected(std::string team_name)
{
  valid_item_ = true;
  team_name_ = team_name;
}

std::string
TeamSelectMenu::get_team_name()
{
  return team_name_;
}

void
TeamSelectMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  //subWindow().bkgd(parent_->getbkgd());
  refresh();
}

int
TeamSelectMenu::det_lines(std::shared_ptr<llsf_msgs::GameInfo> &gameinfo)
{
  return gameinfo->known_teams_size();
}


RobotMaintenanceMenu::RobotMaintenanceMenu(NCursesWindow *parent, Team team,
					   std::shared_ptr<llsf_msgs::RobotInfo> rinfo)
  : Menu(det_lines(team, rinfo) + 1 + 2, det_cols(rinfo) + 2,
	 (parent->lines() - (det_lines(team, rinfo) + 1))/2,
	 (parent->cols() - (det_cols(rinfo) + 2))/2)
{
  valid_item_ = false;
  int n_items = det_lines(team, rinfo);
  items_.resize(n_items);
  int ni = 0;
  //bool has_1 = false, has_2 = false, has_3 = false;
  NCursesMenuItem **mitems = new NCursesMenuItem*[2 + n_items];
  for (int i = 0; i < rinfo->robots_size(); ++i) {
    const llsf_msgs::Robot &r = rinfo->robots(i);
    if (r.team_color() != team) continue;

    std::string s = boost::str(boost::format("%s%s %u %s/%s")
			       % (r.state() == llsf_msgs::MAINTENANCE ? "*" : 
				  (r.state() == llsf_msgs::DISQUALIFIED ? "D" : " "))
			       % (r.maintenance_cycles() > 0 ? "!" : " ")
			       % r.number() % r.name() % r.team());
    items_[ni++] = std::make_tuple(s, r.number(),
				   (r.state() != llsf_msgs::MAINTENANCE &&
				    r.state() != llsf_msgs::DISQUALIFIED));

    /*
    if (r.number() == 1)  has_1 = true;
    if (r.number() == 2)  has_2 = true;
    if (r.number() == 3)  has_3 = true;
    */
  }
  /*
  if (! has_1) {
    items_[ni++] = std::make_tuple("  1 0x Unknown", 1, false);
  }
  if (! has_2) {
    items_[ni++] = std::make_tuple("  2 0x Unknown", 2, false);
  }
  if (! has_3) {
    items_[ni++] = std::make_tuple("  3 0x Unknown", 3, false);
  }
  */

  std::sort(items_.begin(), items_.end(),
	    [](const ItemTuple &i1, const ItemTuple &i2) -> bool
	    {
	      return std::get<1>(i1) < std::get<1>(i2);
	    });

  for (int i = 0; i < ni; ++i) {
    SignalItem *item = new SignalItem(std::get<0>(items_[i]));
    item->signal().connect(boost::bind(&RobotMaintenanceMenu::robot_selected, this,
				       std::get<1>(items_[i]), std::get<2>(items_[i])));
    mitems[i] = item;
  }

  int cols = det_cols(rinfo) - std::string("** CANCEL **").length();
  int cols_half = cols / 2;
  s_cancel_ = "";
  for (int i = 0; i < cols_half; ++i) {
    s_cancel_ += " ";
  }
  s_cancel_ += "** CANCEL **";
  for (int i = 0; i < cols_half; ++i) {
    s_cancel_ += " ";
  }
  if (cols_half * 2 < cols) s_cancel_ += " ";
  mitems[ni] = new SignalItem(s_cancel_);
  mitems[ni+1] = new NCursesMenuItem();

  set_mark("");
  set_format(ni+1, 1);
  InitMenu(mitems, true, true);
  frame("Robot Maintenance");
}

void
RobotMaintenanceMenu::robot_selected(unsigned int number, bool maintenance)
{
  valid_item_ = true;
  robot_number_ = number;
  robot_maintenance_ = maintenance;
}

void
RobotMaintenanceMenu::get_robot(unsigned int &number, bool &maintenance)
{
  if (valid_item_) {
    number = robot_number_;
    maintenance = robot_maintenance_;
  }
}

void
RobotMaintenanceMenu::On_Menu_Init()
{
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));
  refresh();
}

int
RobotMaintenanceMenu::det_lines(Team team, std::shared_ptr<llsf_msgs::RobotInfo> &rinfo)
{
  int rv = 0;
  for (int i = 0; i < rinfo->robots_size(); ++i) {
    const llsf_msgs::Robot &r = rinfo->robots(i);
    if (r.team_color() != team) continue;
    rv += 1;
  }
  return rv;
}

int
RobotMaintenanceMenu::det_cols(std::shared_ptr<llsf_msgs::RobotInfo> &rinfo)
{
  int cols = std::string("** CANCEL **").length();
  for (int i = 0; i < rinfo->robots_size(); ++i) {
    const llsf_msgs::Robot &r = rinfo->robots(i);
    cols = std::max(cols, (int)(r.name().length() + r.team().length() + 7));
  }

  return cols;
}

} // end of namespace llsfrb
