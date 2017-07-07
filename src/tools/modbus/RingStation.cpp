#include "RingStation.h"
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

RingStation::RingStation() : Machine(RING_STATION_CMD) { }
RingStation::~RingStation() {}

/*void RingStation::getRing() {
  lock_guard<mutex> g(lock_);
  sendCommand(RING_STATION_CMD | GET_RING_CMD);
  waitForReady();
}*/

// Need information on how to access this
bool RingStation::ringReady() {
  std::cout << "Not implemented yet!" << std::endl;
  return true;
}

void RingStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_RS);
}

}
}
