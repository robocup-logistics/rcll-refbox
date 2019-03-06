// This file contains various defines.
// They are related to the communication to the PLC.
#pragma once

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

// Base commands for modbus communication:
// Each machine type has a base prefix.
// The sub commands are added to these base prefixes.
enum Station {
  STATION_BASE = 100,
  STATION_RING = 200,
  STATION_CAP = 300,
  STATION_DELIVERY = 400,
  STATION_STORAGE = 500,
};

// Payload for set_type_cmd
enum StationType {
  STATION_TYPE_BS = 1,
  STATION_TYPE_RS = 2,
  STATION_TYPE_CS = 3,
  STATION_TYPE_DS = 4,
  STATION_TYPE_SS = 5,
};

// payload for light command
enum LightState {
  LIGHT_STATE_OFF = 0,
  LIGHT_STATE_ON = 1,
  LIGHT_STATE_BLINK = 2,
};

// all stations, no combining with Station code.
// These commands will be sent, as they are.
enum LightColor {
  LIGHT_COLOR_RESET = 20,
  LIGHT_COLOR_RED = 21,
  LIGHT_COLOR_YELLOW = 22,
  LIGHT_COLOR_GREEN = 23,
};

enum BaseColor {
  BASE_COLOR_RED= 1,
  BASE_COLOR_BLACK = 3,
  BASE_COLOR_SILVER = 2,
};

enum Command {
  COMMAND_NOTHING = 0,
  COMMAND_SET_TYPE = 10,
  COMMAND_RESET = 0,
  COMMAND_MOVE_CONVEYOR = 2,
};

enum SensorOnMPS {
  SENSOR_INPUT = 1,
  SENSOR_OUTPUT = 3,
  SENSOR_MIDDLE = 2,
};

enum Operation {
  OPERATION_GET_BASE = 1,
  OPERATION_WAIT_FOR_BASES = 1,
  OPERATION_MOUNT_RING = 3,
  OPERATION_CAP_ACTION = 1,
  OPERATION_CAP_RETRIEVE = 1,
  OPERATION_CAP_MOUNT = 2,
  OPERATION_DELIVER = 1,
  OPERATION_GET_F_PRODUCT = 1,

};

// The status flags.
// To be combined with bitwise | and read with bitwise &
enum Status {
  STATUS_ENABLED = 4,
  STATUS_ERR = 3,
  STATUS_READY = 2,
  STATUS_BUISY = 1,
};

// TBD: timeout for the band to reach the end position
// in ms
// 0 is no timeout
// NOTE: The timeout shall be handeled on refbox side
enum Timeout {
  TIMEOUT_BAND = 0,
  // TBD: time out for the msp to unset the buisy flag in ms
  // Normally this shall be within one PLC cycle.
  // (a couple of milli seconds).
  // However, due to protocol overhead, I use 1 sec as limit.
  TIMEOUT_BUISY = 1000,
};

}
}
