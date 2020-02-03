/***************************************************************************
 *  machine_factory.h - Create MPS machine instances
 *
 *  Created: Tue 21 Jan 2020 12:53:24 CET 12:53
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

#include <memory>
#include <string>

namespace llsfrb {
namespace mps_comm {
class MachineFactory
{
public:
	MachineFactory() = default;
	std::unique_ptr<Machine> create_machine(const std::string &name,
	                                        const std::string &type,
	                                        const std::string &ip,
	                                        unsigned int       port,
	                                        const std::string &connection_mode);
};
} // namespace mps_comm
} // namespace llsfrb
