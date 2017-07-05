// This file contains the implementation of a cap station.
// A cap station can be used, to mount caps on top of things.
#pragma once

#include "Machine.h"

class CapStation: public Machine {
  public:
    CapStation();

    // -----------------
    // deprecated methods
    void retrieveCap();
    void mountCap();
    // end of deprecated
    // ----------------

    virtual ~CapStation();

    // Handle protobuf message (from refbox)
    void handleProtobufMsg(google::protobuf::Message& m, MachineProtoServer& s);
    // Tell plc, which machine I am
    virtual void identify();
    // Send the reset command
    virtual void reset();
};
