// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  ring_station.cpp - MQTT communication with the RS
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

#include "ring_station.h"

#include "../mps_io_mapping.h"

#include <iostream>

namespace rcll {
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
	std::string m_feeder;
	switch(feeder) {
		case 0:	m_feeder = "RING0"; break;
		case 1: m_feeder = "RING1";	break;
		default: throw std::runtime_error("Invalid feeder number");

	}
	enqueue_instruction("MOUNT_RING " + m_feeder);
}

void
MqttRingStation::register_slide_callback(std::function<void(unsigned int)> callback)
{
	mqtt_client_->register_slide_callback(callback);
}

} // namespace mps_comm
} // namespace rcll
