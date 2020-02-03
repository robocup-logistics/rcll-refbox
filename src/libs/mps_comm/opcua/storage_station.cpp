/***************************************************************************
 *  storage_station.cpp - OPC-UA communication with the SS
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

#include "storage_station.h"

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

OpcUaStorageStation::OpcUaStorageStation(const std::string &name,
                                         const std::string &ip,
                                         unsigned short     port,
                                         ConnectionMode     mode)
: Machine(name), OpcUaMachine(Station::STATION_STORAGE, ip, port, mode)
{
}

void
OpcUaStorageStation::identify()
{
	enqueue_instruction(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_SS);
}

} // namespace mps_comm
} // namespace llsfrb
