#include "CapStation.h"
#include "MPSIoMapping.h"
#include "protobuf/MachineInstructions.pb.h"
#include "MachineProtoServer.h"
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

using namespace llsf_msgs;

#define CASE(type) } else if (dynamic_cast<type *> (&m)) {auto mc = dynamic_cast<type *> (&m);

void CapStation::handleProtobufMsg(google::protobuf::Message& m, MachineProtoServer& s) {
  if (0) {
  CASE(MoveConveyorBelt)
    sendCommand(MOVE_BAND_CMD + CAP_STATION_CMD, mc->stop_sensor(), mc->direction(), // CMD, data1, data2
      TIMEOUT_BAND);
  CASE(CSTask)
    sendCommand(CAP_ACTION_CMD + CAP_STATION_CMD, mc->operation());
  } else {
    Machine::handleProtobufMsg(m, s);
  }
}
