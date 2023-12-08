/***************************************************************************
 *  delivery_station.h - MQTT communication with the DS
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

// This file implements the delivery station
#pragma once

#include "../delivery_station.h"
#include "machine.h"

namespace llsfrb {
namespace mps_comm {

class MqttDeliveryStation : public virtual MqttMachine, public virtual DeliveryStation
{
public:
	MqttDeliveryStation(const std::string &name,
	                    const std::string &ip,
	                    unsigned short     port,
	                    const std::string &log_path = "",
	                    ConnectionMode     mode     = PLC);
	virtual ~MqttDeliveryStation();

	// Send command to deliver a product
	// slot is between 1 and 3
	// Deprecated
	void deliver_product(int slot);
};

} // namespace mps_comm
} // namespace llsfrb
