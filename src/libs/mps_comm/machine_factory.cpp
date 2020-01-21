/***************************************************************************
 *  machine_factory.cpp - Create MPS machine instances
 *
 *  Created: Tue 21 Jan 2020 12:53:24 CET 12:53
 *  Copyright  2020  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "machine_factory.h"

#include <core/exception.h>

namespace llsfrb {
namespace mps_comm {
Machine *
MachineFactory::create_machine(std::string  name,
                               std::string  type,
                               std::string  ip,
                               unsigned int port,
                               std::string  connection_mode)
{
	Machine::ConnectionMode mode;
	if (connection_mode == "plc") {
		mode = Machine::PLC;
	} else if (connection_mode == "simulation") {
		mode = Machine::SIMULATION;
	} else if (connection_mode == "mockup") {
		mode = Machine::MOCKUP;
	} else {
		throw fawkes::Exception("Unexpected connection mode '%s' for machine '%s'",
		                        connection_mode.c_str(),
		                        name.c_str());
	}

	if (type == "BS") {
		return new BaseStation(name, ip, port, mode);
	} else if (type == "CS") {
		return new CapStation(name, ip, port, mode);
	} else if (type == "RS") {
		return new RingStation(name, ip, port, mode);
	} else if (type == "DS") {
		return new DeliveryStation(name, ip, port, mode);
	} else if (type == "SS") {
		return new StorageStation(name, ip, port, mode);
	} else {
		throw fawkes::Exception("Unexpected machine type '%s' for machine '%s'",
		                        type.c_str(),
		                        name.c_str());
	}
}
} // namespace mps_comm
} // namespace llsfrb