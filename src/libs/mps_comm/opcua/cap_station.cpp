/***************************************************************************
 *  cap_station.cpp - OPC-UA communication with the CS
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

#include "cap_station.h"

#include "mps_io_mapping.h"

#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

CapStation::CapStation(std::string name, std::string ip, unsigned short port, ConnectionMode mode)
: Machine(name, Station::STATION_CAP, ip, port, mode)
{
}

CapStation::~CapStation()
{
}

void
CapStation::band_on_until_in()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_,
	             Operation::OPERATION_BAND_IN,
	             ConveyorDirection::BACKWARD);
}

void
CapStation::band_on_until_mid()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_,
	             Operation::OPERATION_BAND_MID,
	             ConveyorDirection::FORWARD);
}

void
CapStation::band_on_until_out()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_,
	             Operation::OPERATION_BAND_OUT,
	             ConveyorDirection::FORWARD);
}

void
CapStation::retrieve_cap()
{
	send_command(Operation::OPERATION_CAP_ACTION + machine_type_, Operation::OPERATION_CAP_RETRIEVE);
}

void
CapStation::mount_cap()
{
	send_command(Operation::OPERATION_CAP_ACTION + machine_type_, Operation::OPERATION_CAP_MOUNT);
}

void
CapStation::identify()
{
	send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_CS);
}

} // namespace mps_comm
} // namespace llsfrb
