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

BaseStation::BaseStation() : Machine(BASE_STATION_CMD) { }
BaseStation::~BaseStation() {}

void BaseStation::getBase(unsigned short color) {
  getBase( color, DIR_OUT);
}

void BaseStation::getBase(unsigned short color, unsigned short side) {
  //lock_guard<mutex> g(lock_);
  sendCommand(machine_type_ + GET_BASE_CMD, color);
  sendCommand(machine_type_ + MOVE_BAND_CMD, side);
  waitForReady();
}

// Need information on how to access this
bool BaseStation::baseReady() {
  std::cout << "Not implemented yet!" << std::endl;
  return true;
}

// Need information on how to access this
bool BaseStation::isEmpty() {
  std::cout << "Not implemented yet!" << std::endl;
  return false;
}

void BaseStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_BS);
}

}
}
