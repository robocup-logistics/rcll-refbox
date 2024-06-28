/***************************************************************************
 *  mps_io_mapping.h - Mappings of named commands to low-level commands
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

// This file contains various defines.
// They are related to the communication to the PLC.
#pragma once

#include "core/exception.h"

namespace rcll {
namespace mps_comm {

enum ConveyorDirection { FORWARD = 1, BACKWARD = 2 };

enum MPSSensor { INPUT = 1, MIDDLE = 2, OUTPUT = 3 };
// Base commands for mps_comm communication:
// Each machine type has a base prefix.
// The sub commands are added to these base prefixes.
enum Station {
	STATION_BASE     = 100,
	STATION_RING     = 200,
	STATION_CAP      = 300,
	STATION_DELIVERY = 400,
	STATION_STORAGE  = 500,
};

// Payload for set_type_cmd
enum StationType {
	STATION_TYPE_BS = 1,
	STATION_TYPE_RS = 2,
	STATION_TYPE_CS = 3,
	STATION_TYPE_DS = 4,
	STATION_TYPE_SS = 5,
};

constexpr StationType
get_type_from_station(const Station &station)
{
	switch (station) {
	case STATION_BASE: return STATION_TYPE_BS;
	case STATION_RING: return STATION_TYPE_RS;
	case STATION_CAP: return STATION_TYPE_CS;
	case STATION_DELIVERY: return STATION_TYPE_DS;
	case STATION_STORAGE: return STATION_TYPE_SS;
	default: throw fawkes::Exception("Unknown machine type: %hi", station);
	}
}

// payload for light command
enum LightState {
	LIGHT_STATE_OFF   = 0,
	LIGHT_STATE_ON    = 1,
	LIGHT_STATE_BLINK = 2,
};

// all stations, no combining with Station code.
// These commands will be sent, as they are.
enum LightColor {
	LIGHT_COLOR_RESET  = 20,
	LIGHT_COLOR_RED    = 21,
	LIGHT_COLOR_YELLOW = 22,
	LIGHT_COLOR_GREEN  = 23,
};

enum BaseColor {
	BASE_COLOR_RED    = 1,
	BASE_COLOR_BLACK  = 3,
	BASE_COLOR_SILVER = 2,
};

enum Command {
	COMMAND_NOTHING       = 0,
	COMMAND_SET_TYPE      = 10,
	COMMAND_RESET         = 0,
	COMMAND_MOVE_CONVEYOR = 2,
};

enum SensorOnMPS {
	SENSOR_INPUT  = 1,
	SENSOR_OUTPUT = 3,
	SENSOR_MIDDLE = 2,
};

enum Operation {
	OPERATION_GET_BASE       = 1,
	OPERATION_WAIT_FOR_BASES = 1,
	OPERATION_MOUNT_RING     = 3,
	OPERATION_CAP_ACTION     = 1,
	OPERATION_CAP_RETRIEVE   = 1,
	OPERATION_CAP_MOUNT      = 2,
	OPERATION_DELIVER        = 1,
	OPERATION_GET_F_PRODUCT  = 1,
	OPERATION_RETRIEVE       = 30,
	OPERATION_STORE          = 40,
	OPERATION_RELOCATE       = 50,
	OPERATION_BAND_ON_UNTIL  = 2,
	OPERATION_BAND_IN        = 1,
	OPERATION_BAND_MID       = 2,
	OPERATION_BAND_OUT       = 3,
};

// The status flags.
// To be combined with bitwise | and read with bitwise &
enum Status {
	STATUS_ENABLED = 4,
	STATUS_ERR     = 3,
	STATUS_READY   = 2,
	STATUS_BUSY    = 1,
};

// TBD: timeout for the band to reach the end position
// in ms
// 0 is no timeout
// NOTE: The timeout shall be handeled on refbox side
enum Timeout {
	TIMEOUT_BAND = 0,
};

} // namespace mps_comm
} // namespace rcll
