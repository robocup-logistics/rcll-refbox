// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  base_station.cpp - MQTT communication with the BS
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

#include "base_station.h"

#include "../mps_io_mapping.h"
#include "core/exception.h"

#include <iostream>

namespace rcll {
namespace mps_comm {

MqttBaseStation::MqttBaseStation(const std::string &name,
                                 const std::string &ip,
                                 unsigned short     port,
                                 const std::string &log_path,
                                 ConnectionMode     mode)
: Machine(name), MqttMachine(name, Station::STATION_BASE, ip, port, log_path, mode)
{
}

void
MqttBaseStation::get_base(llsf_msgs::BaseColor color)
{
	std::string m_color;
	switch (color) {
		case llsf_msgs::BASE_RED: m_color = "RED"; break;
		case llsf_msgs::BASE_BLACK: m_color = "BLACK"; break;
		case llsf_msgs::BASE_SILVER: m_color = "SILVER"; break;
		default: throw fawkes::Exception("Unexpected base color (%lu)", color);
	}

	enqueue_instruction("GET_BASE " + m_color);
}

} // namespace mps_comm
} // namespace rcll
