/***************************************************************************
 *  base_station.h - MQTT communication with the BS
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

// This file contains the BaseStation class.
// At a base station a robot can pick up a base
#pragma once

#include "../base_station.h"
#include "machine.h"
#include "mqtt_utils.h"

namespace llsfrb {
namespace mps_comm {

class MqttBaseStation : public virtual MqttMachine, public virtual BaseStation
{
public:
	MqttBaseStation(const std::string &name,
	                const std::string &ip,
	                unsigned short     port,
	                const std::string &log_path = "",
	                ConnectionMode     mode     = PLC);

	void get_base(llsf_msgs::BaseColor slot) override;
};

} // namespace mps_comm
} // namespace llsfrb
