/***************************************************************************
 *  ring_station.cpp - Mockup ring station
 *
 *  Created: Sat 01 Feb 2020 18:24:25 CET 18:24
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

#include "ring_station.h"

#include "durations.h"

#include <chrono>

namespace llsfrb {
namespace mps_comm {

MockupRingStation::MockupRingStation(const std::string &name) : Machine(name)
{
}

void
MockupRingStation::mount_ring(unsigned int feeder)
{
	callback_busy_(true);
	std::lock_guard<std::mutex> lg(queue_mutex_);
	queue_.push(std::make_tuple([this] { callback_busy_(false); },
	                            std::chrono::system_clock::now()
	                              + std::max(min_operation_duration_,
	                                         std::chrono::round<std::chrono::milliseconds>(
	                                           duration_ring_mount_ / exec_speed_))));
	queue_condition_.notify_one();
}

} // namespace mps_comm
} // namespace llsfrb
