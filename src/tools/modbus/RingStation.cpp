#include "RingStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

RingStation::RingStation() {}
RingStation::~RingStation() {}

/*void RingStation::getRing() {
  lock_guard<mutex> g(lock_);
  sendCommand(RING_STATION_CMD | GET_RING_CMD);
  waitForReady();
}*/

// Need information on how to access this
bool RingStation::ringReady() {
  cout << "Not implemented yet!" << endl;
  return true;
}

void RingStation::reset() {
  sendCommand(RING_STATION_CMD | RESET_CMD);
}

void RingStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_RS);
}
