/***************************************************************************
 *  ring_station.cpp - OPC-UA communication with the RS
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

#include "ring_station.h"

#include "mps_io_mapping.h"

#include <iostream>

namespace llsfrb {
namespace mps_comm {

const std::vector<OpcUtils::MPSRegister>
  OpcUaRingStation::SUB_REGISTERS({OpcUtils::MPSRegister::SLIDECOUNT_IN,
                                   OpcUtils::MPSRegister::BARCODE_IN,
                                   OpcUtils::MPSRegister::ERROR_IN,
                                   OpcUtils::MPSRegister::STATUS_BUSY_IN,
                                   OpcUtils::MPSRegister::STATUS_ENABLE_IN,
                                   OpcUtils::MPSRegister::STATUS_ERROR_IN,
                                   OpcUtils::MPSRegister::STATUS_READY_IN});

OpcUaRingStation::OpcUaRingStation(std::string    name,
                                   std::string    ip,
                                   unsigned short port,
                                   ConnectionMode mode)
: Machine(name), OpcUaMachine(Station::STATION_RING, ip, port, mode)
{
}

// Need information on how to access this
bool
OpcUaRingStation::ring_ready()
{
	std::cout << "Not implemented yet!" << std::endl;
	return true;
}

void
OpcUaRingStation::identify()
{
	enqueue_instruction(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_RS);
}

void
OpcUaRingStation::mount_ring(unsigned int feeder)
{
	enqueue_instruction(Operation::OPERATION_MOUNT_RING + machine_type_, feeder);
}

} // namespace mps_comm
} // namespace llsfrb
