#include "RingStation.h"
#include "MPSIoMapping.h"
#include <iostream>
#include "MachineProtoServer.h"

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

using namespace llsf_msgs;

#define CASE(type) } else if (dynamic_cast<type *> (&m)) {auto mc = dynamic_cast<type *> (&m);

void RingStation::handleProtobufMsg(google::protobuf::Message& m, MachineProtoServer& s) {
  if (0) {
  CASE(MoveConveyorBelt)
    sendCommand(MOVE_BAND_CMD + RING_STATION_CMD, mc->stop_sensor(), mc->direction(), // CMD, data1, data2
      TIMEOUT_BAND);
    // TODO: send MPSProductRetrived, when Product is retrieved.
    // Unfortunately I don't have a msp message for that yet.
    // Furthermore, this should be done in background. How?
    // Idea: Machine gets a member eg list<ProtobufServers*> or so.
    // On updateRegister() -> check a bit.
    // OR:
    // Background thread, which can be activated and deactivated.
    // This thread fights for the mutex and, on a periodical base,
    // checks, weather the product was retrieved.
  CASE(RSMountRing)
    sendCommand(GET_BASE_CMD + BASE_STATION_CMD, mc->feeder() + 1);
  } else {
    Machine::handleProtobufMsg(m, s);
  }
}
