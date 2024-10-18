// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  storage_station.cpp - MQTT communication with the SS
 *
 *  Created: Thu 21 Feb 2023 13:29:11 CET 13:29
 *  Copyright  2023  Dominik Lampel <lampel@student.tugraz.at>
 *
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

#include "../mps_io_mapping.h"
#include <string>

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

MqttStorageStation::MqttStorageStation(const std::string &name,
                                       const std::string &ip,
                                       unsigned short     port,
                                       const std::string &log_path,
                                       ConnectionMode     mode)
: Machine(name), MqttMachine(name, Station::STATION_STORAGE, ip, port, log_path, mode)
{
}

void
MqttStorageStation::retrieve(unsigned int shelf, unsigned int slot)
{
	if (shelf > 5 || slot > 7) {
		throw std::runtime_error("Shelf or Slot out of Range!");
	}
	enqueue_instruction("RETRIEVE " + std::to_string(shelf) + "," + std::to_string(slot));
}

void
MqttStorageStation::store(unsigned int shelf, unsigned int slot)
{
	if (shelf > 5 || slot > 7) {
		throw std::runtime_error("Shelf or Slot out of Range!");
	}
	enqueue_instruction("STORE " + std::to_string(shelf) + "," + std::to_string(slot));
}

void
MqttStorageStation::relocate(unsigned int shelf,
                             unsigned int slot,
                             unsigned int target_shelf,
                             unsigned int target_slot)
{
	if (shelf > 5 || slot > 7) {
		throw std::runtime_error("Shelf or Slot out of Range!");
	}
	if (target_shelf > 5 || target_slot > 7) {
		throw std::runtime_error("Target-Shelf or Target-Slot out of Range!");
	}

	enqueue_instruction("RELOCATE " + std::to_string(shelf) + "," + std::to_string(slot)
						+ " " + std::to_string(target_shelf) + "," + std::to_string(target_slot));
}

} // namespace mps_comm
} // namespace rcll
