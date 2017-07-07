// This file contains the BaseStation class.
// At a base station a robot can pick up a base
#pragma once

#include "Machine.h"

class BaseStation: public Machine {
  public:
    BaseStation();
    virtual ~BaseStation();
    
    // ----------------------------
    // deprecated commands start
    // Send command to get a cap of given color
    // and on given side
    void getBase(unsigned short slot, unsigned short side);
    void getBase(unsigned short slot);
    // Check, if the cap is ready for take away
    bool baseReady();
    // Check, if the last cap was taken -> new order can be processed
    bool isEmpty();
    // deprecated commands end
    // -----------------------------

    // Tell the PLC, which machine it is
    virtual void identify();
    // Send the reset command
    virtual void reset();
};
