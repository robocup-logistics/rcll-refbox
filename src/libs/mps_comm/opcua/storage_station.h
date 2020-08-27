/***************************************************************************
 *  storage_station.h - OPC-UA communication with the SS
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

// Storage Station
// The storage station can be used to store products.
// It will not work as intended, because protocols do not match yet.
#pragma once

#include "../storage_station.h"
#include "machine.h"

namespace llsfrb {
namespace mps_comm {

class OpcUaStorageStation : public virtual OpcUaMachine, public virtual StorageStation
{
public:
	OpcUaStorageStation(const std::string &name,
	                    const std::string &ip,
	                    unsigned short     port,
	                    const std::string &log_path = "",
	                    ConnectionMode     mode     = PLC);
	void retrieve(unsigned int shelf, unsigned int slot) override;
	void store(unsigned int shelf, unsigned int slot) override;
};

} // namespace mps_comm
} // namespace llsfrb
