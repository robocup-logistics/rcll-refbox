// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  delivery_station.cpp - OPC-UA communication with the DS
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

#include "delivery_station.h"

#include "../mps_io_mapping.h"

#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

OpcUaDeliveryStation::OpcUaDeliveryStation(const std::string &name,
                                           const std::string &ip,
                                           unsigned short     port,
                                           const std::string &log_path,
                                           ConnectionMode     mode)
: Machine(name), OpcUaMachine(Station::STATION_DELIVERY, ip, port, log_path, mode)
{
}

OpcUaDeliveryStation::~OpcUaDeliveryStation()
{
}

void
OpcUaDeliveryStation::deliver_product(int slot)
{
	enqueue_instruction(machine_type_ | Operation::OPERATION_DELIVER, slot);
}

} // namespace mps_comm
} // namespace llsfrb
