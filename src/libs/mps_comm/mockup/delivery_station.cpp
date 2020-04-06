/***************************************************************************
 *  delivery_station.cpp - Mockup delivery station
 *
 *  Created: Sat 01 Feb 2020 18:12:57 CET 18:12
 *  Copyright  2020  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include "delivery_station.h"

#include <cmath>

namespace llsfrb {
namespace mps_comm {

MockupDeliveryStation::MockupDeliveryStation(const std::string &name) : Machine(name)
{
}

void
MockupDeliveryStation::deliver_product(int slot)
{
	assert(slot == 1 || slot == 2 || slot == 3);
	callback_busy_(true);
	std::lock_guard<std::mutex> lg(queue_mutex_);
	queue_.push(
	  std::make_tuple([this] { callback_busy_(false); },
	                  std::chrono::system_clock::now()
	                    + std::chrono::seconds(1 + (int)std::ceil(float(slot / exec_speed_)))));
	queue_condition_.notify_one();
}

} // namespace mps_comm
} // namespace llsfrb
