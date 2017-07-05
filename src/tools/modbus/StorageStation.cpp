#include "StorageStation.h"
#include "MPSIoMapping.h"
#include <iostream>
#include "MachineProtoServer.h"

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


using namespace llsf_msgs;

#define CASE(type) } else if (dynamic_cast<type *> (&m)) {auto mc = dynamic_cast<type *> (&m);

void StorageStation::handleProtobufMsg(google::protobuf::Message& m, MachineProtoServer& s) {
  if (0) {
  CASE(SSTask)
    // TODO: this is for sure wrong!
    sendCommand(GET_F_PRODUCT_CMD
        + STORAGE_STATION_CMD, mc->slot().y() + 1, 0, // CMD, data1, data2
      TIMEOUT_BAND);
  } else {
    Machine::handleProtobufMsg(m, s);
  }
}
