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

#include "mockup/stations.h"
#include "opcua/stations.h"

#include <core/exception.h>

namespace llsfrb {
namespace mps_comm {
std::unique_ptr<Machine>
MachineFactory::create_machine(std::string  name,
                               std::string  type,
                               std::string  ip,
                               unsigned int port,
                               std::string  connection_mode)
{
	OpcUaMachine::ConnectionMode mode;
	if (connection_mode == "plc") {
		mode = OpcUaMachine::PLC;
	} else if (connection_mode == "simulation") {
		mode = OpcUaMachine::SIMULATION;
	} else if (connection_mode == "mockup") {
		mode = OpcUaMachine::MOCKUP;
	} else {
		throw fawkes::Exception("Unexpected connection mode '%s' for machine '%s'",
		                        connection_mode.c_str(),
		                        name.c_str());
	}

	if (connection_mode == "plc") {
		std::unique_ptr<OpcUaMachine> mps;
		if (type == "BS") {
			mps = std::make_unique<OpcUaBaseStation>(name, ip, port, mode);
		} else if (type == "CS") {
			mps = std::make_unique<OpcUaCapStation>(name, ip, port, mode);
		} else if (type == "RS") {
			mps = std::make_unique<OpcUaRingStation>(name, ip, port, mode);
		} else if (type == "DS") {
			mps = std::make_unique<OpcUaDeliveryStation>(name, ip, port, mode);
		} else if (type == "SS") {
			mps = std::make_unique<OpcUaStorageStation>(name, ip, port, mode);
		} else {
			throw fawkes::Exception("Unexpected machine type '%s' for machine '%s'",
			                        type.c_str(),
			                        name.c_str());
		}
		mps->connect();
		return std::move(mps);
	} else if (connection_mode == "mockup") {
		if (type == "BS") {
			return std::make_unique<MockupBaseStation>(name);
		} else if (type == "CS") {
			return std::make_unique<MockupCapStation>(name);
		} else {
			throw fawkes::Exception(
			  "Unexpected machine type '%s' for machine '%s' and connection mode '%s'",
			  type.c_str(),
			  name.c_str(),
			  connection_mode.c_str());
		}
	} else {
		throw fawkes::Exception("Unexpected connection mode '%s' for machine '%s'",
		                        connection_mode.c_str(),
		                        name.c_str());
	}
}
} // namespace mps_comm
} // namespace llsfrb
