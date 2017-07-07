#include "DeliveryStation.h"
#include "MPSIoMapping.h"
#include <iostream>

using namespace std;

DeliveryStation::DeliveryStation() : Machine(DELIVERY_STATION_CMD) { }
DeliveryStation::~DeliveryStation() {}

void DeliveryStation::deliverProduct(int slot) {
  //lock_guard<mutex> g(lock_);
  sendCommand(machine_type_ | DELIVER_CMD, slot);
  waitForReady();
}

void DeliveryStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_DS);
}
