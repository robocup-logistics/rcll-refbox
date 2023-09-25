/***************************************************************************
 *  ring_station.cpp - OPC-UA communication with the RS
 *
 *  Created: Thu 21 Feb 2019 13:29:11 CET 13:29
 *  Copyright  2019  Alex Maestrini <maestrini@student.tugraz.at>
 *                   Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "../mps_io_mapping.h"

#include <iostream>

namespace llsfrb {
namespace mps_comm {

MqttRingStation::MqttRingStation(const std::string &name,
                                   const std::string &ip,
                                   unsigned short     port,
                                   const std::string &log_path,
                                   ConnectionMode     mode)
: Machine(name), MqttMachine(name, Station::STATION_RING, ip, port, log_path, mode)
{
}

void
MqttRingStation::mount_ring(unsigned int feeder, llsf_msgs::RingColor color)
{
	uint16_t payload2 = color;
	enqueue_instruction(Operation::OPERATION_MOUNT_RING + machine_type_, feeder, payload2);
}

void
MqttRingStation::register_slide_callback(std::function<void(unsigned int)> callback)
{
 	mqtt_client_->register_slide_callback(callback);	
}

} // namespace mps_comm
} // namespace llsfrb
