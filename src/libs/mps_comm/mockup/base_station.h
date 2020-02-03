/***************************************************************************
 *  base_station.h - Mockup communication with the base station
 *
 *  Created: Mon 27 Jan 2020 21:20:40 CET 21:20
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

#include "../base_station.h"
#include "machine.h"

namespace llsfrb {
namespace mps_comm {
class MockupBaseStation : public virtual MockupMachine, public virtual BaseStation
{
public:
	MockupBaseStation(const std::string &name);
	void get_base(llsf_msgs::BaseColor slot) override;
	void identify() override{};
};
} // namespace mps_comm
} // namespace llsfrb
