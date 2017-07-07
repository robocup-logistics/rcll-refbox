#include "BaseStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

BaseStation::BaseStation() {}
BaseStation::~BaseStation() {}

void BaseStation::getBase(unsigned short color) {
  getBase( color, DIR_OUT);
}

void BaseStation::getBase(unsigned short color, unsigned short side) {
  //lock_guard<mutex> g(lock_);
  sendCommand(BASE_STATION_CMD + GET_BASE_CMD, color);
  sendCommand(BASE_STATION_CMD + MOVE_BAND_CMD, side);
  waitForReady();
}

// Need information on how to access this
bool BaseStation::baseReady() {
  cout << "Not implemented yet!" << endl;
  return true;
}

// Need information on how to access this
bool BaseStation::isEmpty() {
  cout << "Not implemented yet!" << endl;
  return false;
}

void BaseStation::reset() {
  sendCommand(BASE_STATION_CMD | RESET_CMD);
}

void BaseStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_BS);
}
