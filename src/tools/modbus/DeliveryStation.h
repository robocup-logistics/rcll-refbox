// This file implements the delivery station
#pragma once

#include "Machine.h"
#include "protobuf/MachineInstructions.pb.h"

class DeliveryStation: public Machine {
  public:
    DeliveryStation();
    virtual ~DeliveryStation();

    // Send command to deliver a product
    // slot is between 1 and 3
    // Deprecated
    void deliverProduct(int slot);

    // Handle protobuf message (from refbox)
    void handleProtobufMsg(google::protobuf::Message& m, MachineProtoServer& s);

    // Tell PLC, which machine it is
    virtual void identify();
    virtual void reset();
};
