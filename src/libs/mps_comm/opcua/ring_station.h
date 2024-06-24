/***************************************************************************
 *  ring_station.h - OPC-UA communication with the RS
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

// The ring station mounts rings on bases
#pragma once

#include "../ring_station.h"
#include "machine.h"

namespace rcll {
namespace mps_comm {

class OpcUaRingStation : public virtual OpcUaMachine, public virtual RingStation
{
	static const std::vector<OpcUtils::MPSRegister> SUB_REGISTERS;

public:
	OpcUaRingStation(const std::string &name,
	                 const std::string &ip,
	                 unsigned short     port,
	                 const std::string &log_path = "",
	                 ConnectionMode     mode     = PLC);

	void mount_ring(unsigned int feeder, llsf_msgs::RingColor color) override;
	void register_slide_callback(std::function<void(unsigned int)>) override;
};

} // namespace mps_comm
} // namespace rcll
