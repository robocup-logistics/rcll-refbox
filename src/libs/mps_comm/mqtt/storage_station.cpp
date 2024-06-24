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

#include <iostream>

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
	enqueue_instruction(Operation::OPERATION_RETRIEVE + machine_type_, shelf, slot);
}

void
MqttStorageStation::store(unsigned int shelf, unsigned int slot)
{
	enqueue_instruction(Operation::OPERATION_STORE + machine_type_, shelf, slot);
}

void
MqttStorageStation::relocate(unsigned int shelf,
                             unsigned int slot,
                             unsigned int target_shelf,
                             unsigned int target_slot)
{
	enqueue_instruction(Operation::OPERATION_RELOCATE + machine_type_,
	                    (shelf * 10) + slot,
	                    (target_shelf * 10) + target_slot);
}

} // namespace mps_comm
} // namespace rcll
