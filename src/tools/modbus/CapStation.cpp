#include "CapStation.h"
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

CapStation::CapStation() : Machine(CAP_STATION_CMD) { }
CapStation::~CapStation() {}

void CapStation::retrieveCap() {
  sendCommand(CAP_ACTION_CMD + machine_type_, CAP_RETRIEVE);
}

void CapStation::mountCap() {
  sendCommand(CAP_ACTION_CMD + machine_type_, CAP_MOUNT);
}

void CapStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_CS);
}

}
}
