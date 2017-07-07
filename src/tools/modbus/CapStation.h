// This file contains the implementation of a cap station.
// A cap station can be used, to mount caps on top of things.
#pragma once

#include "Machine.h"

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
    void retrieveCap();
    void mountCap();
    // end of deprecated
    // ----------------

    virtual ~CapStation();

    // Tell plc, which machine I am
    virtual void identify();
};

}
}
