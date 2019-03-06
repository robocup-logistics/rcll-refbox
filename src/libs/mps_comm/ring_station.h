// The ring station mounts rings on bases
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

class RingStation: public Machine {
  static const std::vector<OpcUtils::MPSRegister> SUB_REGISTERS;
  public:
    RingStation();
    virtual ~RingStation();

    // Send command to get a ring
    // void getRing();
    // Check, if the cap is ready for take away
    // deprecated
    bool ring_ready();

    void mount_ring(unsigned int feeder);

    // identify: tell PLC, which machine it is running on
    virtual void identify();
};

}
}
