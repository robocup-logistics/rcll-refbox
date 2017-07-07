#include "StorageStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

StorageStation::StorageStation() : Machine(STORAGE_STATION_CMD) { }
StorageStation::~StorageStation() {}

void StorageStation::getProduct(int slot) {
  //lock_guard<mutex> g(lock_);
  sendCommand(machine_type_ + GET_F_PRODUCT_CMD, slot);
  waitForReady();
}

void StorageStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_SS);
}
