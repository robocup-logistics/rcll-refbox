/***************************************************************************
 *  cap_station.h - Abstract cap station interface
 *
 *  Created: Thu 23 Jan 2020 17:05:21 CET 17:05
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
#include "machine.h"

namespace rcll {
namespace mps_comm {

class CapStation : public virtual Machine
{
public:
	virtual void retrieve_cap() = 0;
	virtual void mount_cap()    = 0;
};
} // namespace mps_comm
} // namespace rcll
