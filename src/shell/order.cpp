
/***************************************************************************
 *  order.cpp - LLSF RefBox shell - order
 *
 *  Created: Mon Feb 25 11:49:50 2013
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

#include "order.h"

#include <cstring>
#include <unistd.h>

#include <cursesp.h>

#include "colors.h"

namespace llsfrb_shell {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

LLSFRefBoxShellOrder::LLSFRefBoxShellOrder(int begin_y, int begin_x)
  : NCursesPanel(1, 31, begin_y, begin_x),
    id_(0), complexity_(llsf_msgs::Order::C0), competitive_(false),
    quantity_requested_(0),
    quantity_delivered_cyan_(0), quantity_delivered_magenta_(0),
    delivery_period_begin_(0), delivery_period_end_(0),
    delivery_gate_(1), game_time_(0.)
{
}


LLSFRefBoxShellOrder::~LLSFRefBoxShellOrder()
{
}


void
LLSFRefBoxShellOrder::update(unsigned int id, llsf_msgs::Order::Complexity complexity,
			     bool competitive,
			     llsf_msgs::BaseColor base_color,
			     std::vector<llsf_msgs::RingColor> &ring_colors,
			     llsf_msgs::CapColor  cap_color,
			     unsigned int quantity_requested,
			     unsigned int quantity_delivered_cyan,
			     unsigned int quantity_delivered_magenta,
			     unsigned int delivery_period_begin,
			     unsigned int delivery_period_end,
			     unsigned int delivery_gate)
{
  id_ = id;
  complexity_  = complexity;
  competitive_ = competitive;
  base_color_  = base_color;
  ring_colors_ = ring_colors;
  cap_color_   = cap_color;
  quantity_requested_ = quantity_requested;
  quantity_delivered_cyan_ = quantity_delivered_cyan;
  quantity_delivered_magenta_ = quantity_delivered_magenta;
  delivery_period_begin_ = delivery_period_begin;
  delivery_period_end_ = delivery_period_end;
  delivery_gate_ = delivery_gate;
}


void
LLSFRefBoxShellOrder::set_gametime(unsigned int game_time)
{
  game_time_ = game_time;
  refresh();
}


void
LLSFRefBoxShellOrder::reset()
{
  id_ = 0;
  complexity_ = llsf_msgs::Order::C0;
  quantity_requested_ = 0;
  quantity_delivered_cyan_ = 0;
  quantity_delivered_magenta_ = 0;
  delivery_period_begin_ = 0;
  delivery_period_end_ = 0;
  delivery_gate_ = 0;
  refresh();
}

int
LLSFRefBoxShellOrder::refresh()
{
  standend();
  erase();
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));

  unsigned int col = 0;
  if (id_ != 0) {
    unsigned int begin_min = delivery_period_begin_ / 60;
    unsigned int begin_sec = delivery_period_begin_ - begin_min * 60;
    unsigned int end_min = delivery_period_end_ / 60;
    unsigned int end_sec = delivery_period_end_ - end_min * 60;

    attron(A_BOLD);
    printw(0, id_ >= 10 ? col : col + 1, "%u.", id_);
    col += 2;
    attroff(A_BOLD);

    if (game_time_ >= delivery_period_begin_ && game_time_ <= delivery_period_end_) {
      attron(A_BOLD);
    }

    if (quantity_delivered_cyan_ > 0) {
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_CYAN));
    } else {
      attron(' '|COLOR_PAIR(COLOR_CYAN_ON_BACK));
    }
    printw(0, ++col, "%u", quantity_delivered_cyan_);

    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
    addstr(0, ++col, "/");

    if (quantity_delivered_magenta_ > 0) {
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_MAGENTA));
    } else {
      attron(' '|COLOR_PAIR(COLOR_MAGENTA_ON_BACK));
    }
    printw(0, ++col, "%u", quantity_delivered_magenta_);

    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
    printw(0, ++col, "/%u", quantity_requested_);
    col += 2;

    if (competitive_) {
      attron(' '|COLOR_PAIR(COLOR_YELLOW_ON_BACK));
      printw(0, ++col, "C");
    } else {
      ++col;
	}

    col += 1;

    switch (base_color_) {
    case llsf_msgs::BASE_RED:
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_RED));   break;
    case llsf_msgs::BASE_SILVER:
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE)); break;
    case llsf_msgs::BASE_BLACK:
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLACK));   break;
    }
    addstr(0, ++col, " ");

    if (ring_colors_.size() >= 1) {
      switch (ring_colors_[0]) {
      case llsf_msgs::RING_BLUE:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLUE)); break;
      case llsf_msgs::RING_GREEN:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN)); break;
      case llsf_msgs::RING_ORANGE:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_ORANGE)); break;
      case llsf_msgs::RING_YELLOW:
	attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW)); break;
      }
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    }
    addstr(0, ++col, " ");

    if (ring_colors_.size() >= 2) {
      switch (ring_colors_[1]) {
      case llsf_msgs::RING_BLUE:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLUE)); break;
      case llsf_msgs::RING_GREEN:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN)); break;
      case llsf_msgs::RING_ORANGE:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_ORANGE)); break;
      case llsf_msgs::RING_YELLOW:
	attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW)); break;
      }
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    }
    addstr(0, ++col, " ");

    if (ring_colors_.size() >= 3) {
      switch (ring_colors_[2]) {
      case llsf_msgs::RING_BLUE:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLUE)); break;
      case llsf_msgs::RING_GREEN:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_GREEN)); break;
      case llsf_msgs::RING_ORANGE:
	attron(' '|COLOR_PAIR(COLOR_WHITE_ON_ORANGE)); break;
      case llsf_msgs::RING_YELLOW:
	attron(' '|COLOR_PAIR(COLOR_BLACK_ON_YELLOW)); break;
      }
    } else {
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    }
    addstr(0, ++col, " ");

    switch (cap_color_) {
    case llsf_msgs::CAP_BLACK:
      attron(' '|COLOR_PAIR(COLOR_WHITE_ON_BLACK));   break;
    case llsf_msgs::CAP_GREY:
      attron(' '|COLOR_PAIR(COLOR_BLACK_ON_WHITE)); break;
    }
    addstr(0, ++col, " ");

    col += 1;

    attron(' '|COLOR_PAIR(COLOR_BLACK_ON_BACK));
    printw(0, ++col, "%02u:%02u-%02u:%02u D%u",
	   begin_min, begin_sec, end_min, end_sec,
	   delivery_gate_);
  }

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
