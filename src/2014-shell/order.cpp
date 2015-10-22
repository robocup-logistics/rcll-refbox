
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
  : NCursesPanel(1, 27, begin_y, begin_x),
    id_(0), product_(llsf_msgs::Order::P1), quantity_requested_(0),
    quantity_delivered_(0), delivery_period_begin_(0), delivery_period_end_(0),
    delivery_gate_(llsf_msgs::Order::ANY), game_time_(0.)
{
}


LLSFRefBoxShellOrder::~LLSFRefBoxShellOrder()
{
}


void
LLSFRefBoxShellOrder::update(unsigned int id, llsf_msgs::Order::ProductType product,
			     unsigned int quantity_requested,
			     unsigned int quantity_delivered,
			     unsigned int delivery_period_begin,
			     unsigned int delivery_period_end,
			     llsf_msgs::Order::DeliveryGate delivery_gate)
{
  id_ = id;
  product_ = product;
  quantity_requested_ = quantity_requested;
  quantity_delivered_ = quantity_delivered;
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
  product_ = llsf_msgs::Order::P1;
  quantity_requested_ = 0;
  quantity_delivered_ = 0;
  delivery_period_begin_ = 0;
  delivery_period_end_ = 0;
  delivery_gate_ = llsf_msgs::Order::ANY;
  refresh();
}

int
LLSFRefBoxShellOrder::refresh()
{
  standend();
  erase();
  bkgd(' '|COLOR_PAIR(COLOR_DEFAULT));

  if (id_ != 0) {
    unsigned int begin_min = delivery_period_begin_ / 60;
    unsigned int begin_sec = delivery_period_begin_ - begin_min * 60;
    unsigned int end_min = delivery_period_end_ / 60;
    unsigned int end_sec = delivery_period_end_ - end_min * 60;

    attron(A_BOLD);
    printw("%u.", id_);
    attroff(A_BOLD);

    if (game_time_ >= delivery_period_begin_ && game_time_ <= delivery_period_end_) {
      attron(A_BOLD);
    }
    printw(0, 3, "%u/%u %s %02u:%02u-%02u:%02u %s",
	   quantity_delivered_, quantity_requested_,
	   llsf_msgs::Order::ProductType_Name(product_).c_str(),
	   begin_min, begin_sec, end_min, end_sec,
	   llsf_msgs::Order::DeliveryGate_Name(delivery_gate_).c_str());
  }

  return NCursesPanel::refresh();
}


} // end of namespace llsfrb_shell
