// This file contains the BaseStation class.
// At a base station a robot can pick up a base
#pragma once

#include "machine.h"

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

class BaseStation: public Machine {
  public:
    BaseStation();
    virtual ~BaseStation();
    
    // ----------------------------
    // deprecated commands start
    void band_on_until_in();
    void band_on_until_mid();
    void band_on_until_out();
    // Send command to get a cap of given color
    // and on given side
    void get_base(llsf_msgs::BaseColor slot);
    // Check, if the cap is ready for take away
    bool base_ready();
    // Check, if the last cap was taken -> new order can be processed
    bool is_empty();
    // deprecated commands end
    // -----------------------------

    // Tell the PLC, which machine it is
    virtual void identify();
};

}
}
