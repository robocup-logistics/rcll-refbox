#include "CapStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

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
