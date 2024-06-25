/***************************************************************************
 *  storage_station.h - Mockup for storage station
 *
 *  Created: Sat 01 Feb 2020 18:41:45 CET 18:41
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

#pragma once

#include "../storage_station.h"
#include "machine.h"

namespace llsfrb {
namespace mps_comm {

class MockupStorageStation : public virtual MockupMachine, public virtual StorageStation
{
public:
	MockupStorageStation(const std::string &name, float exec_speed);
	void retrieve(unsigned int shelf, unsigned int slot) override;
	void store(unsigned int shelf, unsigned int slot) override;
	void relocate(unsigned int shelf,
	              unsigned int slot,
	              unsigned int target_shelf,
	              unsigned int target_slot) override;
	void identify() override {};

private:
	void storage_op();
};
} // namespace mps_comm
} // namespace llsfrb
