#include "BaseStation.h"
#include "MPSIoMapping.h"
#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

BaseStation::BaseStation() : Machine(Station::STATION_BASE) { }
BaseStation::~BaseStation() {}

void BaseStation::get_base(unsigned short color) {
  get_base( color, SensorOnMPS::SENSOR_OUTPUT);
}

void BaseStation::get_base(unsigned short color, unsigned short side) {
  //lock_guard<mutex> g(lock_);
  send_command(machine_type_ + Operation::OPERATION_GET_BASE, color);
  send_command(machine_type_ + Command::COMMAND_MOVE_CONVEYOR, side);
  wait_for_ready();
}

// Need information on how to access this
bool BaseStation::base_ready() {
  std::cout << "Not implemented yet!" << std::endl;
  return true;
}

// Need information on how to access this
bool BaseStation::is_empty() {
  std::cout << "Not implemented yet!" << std::endl;
  return false;
}

void BaseStation::identify() {
  send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_BS);
}

}
}
