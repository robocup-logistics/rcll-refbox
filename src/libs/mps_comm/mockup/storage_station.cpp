/***************************************************************************
 *  storage_station.cpp - Mockup for storage station
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

#include "storage_station.h"

namespace llsfrb {
namespace mps_comm {

MockupStorageStation::MockupStorageStation(const std::string &name) : Machine(name)
{
}

} // namespace mps_comm
} // namespace llsfrb
