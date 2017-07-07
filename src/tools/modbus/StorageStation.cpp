#include "StorageStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

StorageStation::StorageStation() {}
StorageStation::~StorageStation() {}

void StorageStation::getProduct(int slot) {
  //lock_guard<mutex> g(lock_);
  sendCommand(STORAGE_STATION_CMD + GET_F_PRODUCT_CMD, slot);
  waitForReady();
}

void StorageStation::reset() {
  sendCommand(STORAGE_STATION_CMD + RESET_CMD);
}

void StorageStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_SS);
}
