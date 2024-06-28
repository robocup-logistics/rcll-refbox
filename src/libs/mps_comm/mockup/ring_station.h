/***************************************************************************
 *  ring_station.h - Mockup ring station
 *
 *  Created: Sat 01 Feb 2020 18:23:59 CET 18:23
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

#include "../ring_station.h"
#include "machine.h"

namespace llsfrb {
namespace mps_comm {

class MockupRingStation : public virtual MockupMachine, public virtual RingStation
{
public:
	MockupRingStation(const std::string &name, float exec_speed);
	void mount_ring(unsigned int, llsf_msgs::RingColor) override;
	void register_slide_callback(std::function<void(unsigned int)> callback) override {};
	void identify() override {};
};

} // namespace mps_comm
} // namespace llsfrb
