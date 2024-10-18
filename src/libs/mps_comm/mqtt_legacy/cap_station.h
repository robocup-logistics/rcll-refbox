/***************************************************************************
 *  cap_station.h - MQTT_LEGACY communication with the CS
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

// This file contains the implementation of a cap station.
// A cap station can be used, to mount caps on top of things.
#pragma once

#include "../cap_station.h"
#include "machine.h"

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

class MqttLegacyCapStation : public virtual MqttLegacyMachine, public virtual CapStation
{
public:
	MqttLegacyCapStation(const std::string &name,
	               const std::string &ip,
	               unsigned short     port,
	               const std::string &log_path = "",
	               ConnectionMode     mode     = MQTTLEGACY);

	virtual ~MqttLegacyCapStation();

	void retrieve_cap() override;
	void mount_cap() override;
};

} // namespace mps_comm
} // namespace rcll
