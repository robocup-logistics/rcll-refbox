// This file contains the implementation of a cap station.
// A cap station can be used, to mount caps on top of things.
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

class CapStation: public Machine {
  public:
    CapStation();

    // -----------------
    // deprecated methods
    void band_on_until_in();
    void band_on_until_mid();
    void band_on_until_out();
    void retrieve_cap();
    void mount_cap();
    // end of deprecated
    // ----------------

    virtual ~CapStation();

    // Tell plc, which machine I am
    virtual void identify();
};

}
}
