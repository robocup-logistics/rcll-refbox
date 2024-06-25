// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  storage_station.cpp - Mockup for storage station
 *
 *  Created: Sat 01 Feb 2020 18:41:45 CET 18:41
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

#include "storage_station.h"

#include "durations.h"

#include <chrono>

namespace llsfrb {
namespace mps_comm {

MockupStorageStation::MockupStorageStation(const std::string &name, float exec_speed)
: MockupMachine(name, exec_speed)
{
}

void
MockupStorageStation::retrieve(unsigned int shelf, unsigned int slot)
{
	storage_op();
}

void
MockupStorageStation::store(unsigned int shelf, unsigned int slot)
{
	storage_op();
}

void
MockupStorageStation::relocate(unsigned int shelf,
                               unsigned int slot,
                               unsigned int target_shelf,
                               unsigned int target_slot)
{
	storage_op();
}

void
MockupStorageStation::storage_op()
{
	callback_busy_(true);
	std::lock_guard<std::mutex> lg(queue_mutex_);
	queue_.push(std::make_tuple([this] { callback_busy_(false); },
	                            std::chrono::system_clock::now()
	                              + std::max(min_operation_duration_,
	                                         std::chrono::round<std::chrono::milliseconds>(
	                                           duration_storage_op_ / exec_speed_))));
	queue_condition_.notify_one();
}

} // namespace mps_comm
} // namespace llsfrb
