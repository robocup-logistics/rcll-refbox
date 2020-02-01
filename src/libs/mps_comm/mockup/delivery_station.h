/***************************************************************************
 *  delivery_station.h - Mockup delivery station
 *
 *  Created: Sat 01 Feb 2020 18:12:41 CET 18:12
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

#include "../delivery_station.h"
#include "machine.h"

namespace llsfrb {
namespace mps_comm {

class MockupDeliveryStation : public virtual MockupMachine, public virtual DeliveryStation
{
public:
	MockupDeliveryStation(std::string name);
	void deliver_product(int slot) override;
	void identify() override{};
};

} // namespace mps_comm
} // namespace llsfrb
