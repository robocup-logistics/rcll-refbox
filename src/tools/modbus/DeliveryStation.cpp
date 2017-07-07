#include "DeliveryStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

DeliveryStation::DeliveryStation() {}
DeliveryStation::~DeliveryStation() {}

void DeliveryStation::deliverProduct(int slot) {
  //lock_guard<mutex> g(lock_);
  sendCommand(DELIVERY_STATION_CMD | DELIVER_CMD, slot);
  waitForReady();
}

void DeliveryStation::reset() {
  sendCommand(DELIVERY_STATION_CMD | RESET_CMD);
}

void DeliveryStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_DS);
}
