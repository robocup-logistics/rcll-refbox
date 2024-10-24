/***************************************************************************
 *  ring_station.h - MQTT_LEGACY communication with the RS
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

// The ring station mounts rings on bases
#pragma once

#include "../ring_station.h"
#include "machine.h"
#include "mqtt_callback.h"
#include "mqtt_client_wrapper.h"

namespace rcll {
namespace mps_comm {

class MqttLegacyRingStation : public virtual MqttLegacyMachine, public virtual RingStation
{
public:
	MqttLegacyRingStation(const std::string &name,
	                const std::string &ip,
	                unsigned short     port,
	                const std::string &log_path = "",
	                ConnectionMode     mode     = MQTTLEGACY);

	void mount_ring(unsigned int feeder, llsf_msgs::RingColor color) override;
	void register_slide_callback(std::function<void(unsigned int)>) override;
};

} // namespace mps_comm
} // namespace rcll
