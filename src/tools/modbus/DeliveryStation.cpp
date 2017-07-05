#include "DeliveryStation.h"
#include "MPSIoMapping.h"
#include "MachineProtoServer.h"
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

using namespace llsf_msgs;

#define CASE(type) } else if (dynamic_cast<type *> (&m)) {auto mc = dynamic_cast<type *> (&m);

void DeliveryStation::handleProtobufMsg(google::protobuf::Message& m, MachineProtoServer& s) {
  if (0) {
  CASE(DSActivateGate)
    deliverProduct(mc->gate() + 1);
  } else {
    Machine::handleProtobufMsg(m, s);
  }
}
