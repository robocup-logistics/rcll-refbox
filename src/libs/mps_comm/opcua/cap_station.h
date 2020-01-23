/***************************************************************************
 *  cap_station.h - OPC-UA communication with the CS
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

// This file contains the implementation of a cap station.
// A cap station can be used, to mount caps on top of things.
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

class OpcUaCapStation : public OpcUaMachine
{
public:
	OpcUaCapStation(std::string name, std::string ip, unsigned short port, ConnectionMode mode);

	// -----------------
	// deprecated methods
	void band_on_until_in();
	void band_on_until_mid();
	void band_on_until_out();
	void retrieve_cap();
	void mount_cap();
	// end of deprecated
	// ----------------

	virtual ~OpcUaCapStation();

	// Tell plc, which machine I am
	virtual void identify();
};

} // namespace mps_comm
} // namespace llsfrb
