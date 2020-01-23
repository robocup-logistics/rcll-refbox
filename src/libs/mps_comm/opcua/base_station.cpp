/***************************************************************************
 *  base_station.cpp - OPC-UA communication with the BS
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

#include "base_station.h"

#include "mps_io_mapping.h"

#include <iostream>

namespace llsfrb {
namespace mps_comm {

OpcUaBaseStation::OpcUaBaseStation(std::string    name,
                                   std::string    ip,
                                   unsigned short port,
                                   ConnectionMode mode)
: OpcUaMachine(name, Station::STATION_BASE, ip, port, mode)
{
}

OpcUaBaseStation::~OpcUaBaseStation()
{
}

void
OpcUaBaseStation::band_on_until_in()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_IN);
}

void
OpcUaBaseStation::band_on_until_mid()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_MID);
}

void
OpcUaBaseStation::band_on_until_out()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_OUT);
}

void
OpcUaBaseStation::get_base(llsf_msgs::BaseColor color)
{
	//lock_guard<mutex> g(lock_);
	llsfrb::mps_comm::BaseColor color_sps;
	;
	switch (color) {
	case llsf_msgs::BASE_RED: color_sps = llsfrb::mps_comm::BaseColor::BASE_COLOR_RED; break;
	case llsf_msgs::BASE_BLACK: color_sps = llsfrb::mps_comm::BaseColor::BASE_COLOR_BLACK; break;
	case llsf_msgs::BASE_SILVER: color_sps = llsfrb::mps_comm::BaseColor::BASE_COLOR_SILVER; break;
	default: std::cout << "Error in RefBox, NOT implemented base color" << std::endl; return;
	}

	send_command(machine_type_ + Operation::OPERATION_GET_BASE, color_sps);
}

// Need information on how to access this
bool
OpcUaBaseStation::base_ready()
{
	std::cout << "Not implemented yet!" << std::endl;
	return true;
}

// Need information on how to access this
bool
OpcUaBaseStation::is_empty()
{
	std::cout << "Not implemented yet!" << std::endl;
	return false;
}

void
OpcUaBaseStation::identify()
{
	send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_BS);
}

} // namespace mps_comm
} // namespace llsfrb
