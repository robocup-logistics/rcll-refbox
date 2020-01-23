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

#include "machine.h"

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

class OpcUaRingStation : public OpcUaMachine
{
	static const std::vector<OpcUtils::MPSRegister> SUB_REGISTERS;

public:
	OpcUaRingStation(std::string name, std::string ip, unsigned short port, ConnectionMode mode);
	virtual ~OpcUaRingStation();

	// Send command to get a ring
	// void getRing();
	// Check, if the cap is ready for take away
	// deprecated
	void band_on_until_in();
	void band_on_until_mid();
	void band_on_until_out();
	bool ring_ready();

	void mount_ring(unsigned int feeder);

	// identify: tell PLC, which machine it is running on
	virtual void identify();
};

} // namespace mps_comm
} // namespace llsfrb
