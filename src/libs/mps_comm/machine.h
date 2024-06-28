/***************************************************************************
 *  machine.h - Abstract MPS machine interface
 *
 *  Created: Thu 23 Jan 2020 15:53:04 CET 15:53
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

#include <msgs/MachineDescription.pb.h>

#include <functional>
#include <string>

namespace llsfrb {
namespace mps_comm {

class Machine
{
public:
	enum ConnectionMode {
		MOCKUP,
		SIMULATION,
		PLC,
		MQTT,
	};

	enum ConveyorDirection { FORWARD = 1, BACKWARD = 2 };
	enum MPSSensor { INPUT = 1, MIDDLE = 2, OUTPUT = 3 };
	Machine() {};
	Machine(const std::string &name) : name_(name)
	{
	}
	virtual ~Machine() {};

	virtual void set_light(llsf_msgs::LightColor color,
	                       llsf_msgs::LightState state = llsf_msgs::ON,
	                       unsigned short        time  = 0)                            = 0;
	virtual void conveyor_move(ConveyorDirection direction, MPSSensor sensor)  = 0;
	virtual void reset_light()                                                 = 0;
	virtual void reset()                                                       = 0;
	virtual void register_busy_callback(std::function<void(bool)>)             = 0;
	virtual void register_ready_callback(std::function<void(bool)>)            = 0;
	virtual void register_barcode_callback(std::function<void(unsigned long)>) = 0;
	virtual std::string
	name() const
	{
		return name_;
	};

protected:
	const std::string name_;
};
} // namespace mps_comm
} // namespace llsfrb
