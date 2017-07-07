#include "CapStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

CapStation::CapStation() {}
CapStation::~CapStation() {}

void CapStation::reset() {
  sendCommand(CAP_STATION_CMD + RESET_CMD);
}

void CapStation::retrieveCap() {
  sendCommand(CAP_ACTION_CMD + CAP_STATION_CMD, CAP_RETRIEVE);
}

void CapStation::mountCap() {
  sendCommand(CAP_ACTION_CMD + CAP_STATION_CMD, CAP_MOUNT);
}

void CapStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_CS);
}
