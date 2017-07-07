// This file implements the delivery station
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

class DeliveryStation: public Machine {
  public:
    DeliveryStation();
    virtual ~DeliveryStation();

    // Send command to deliver a product
    // slot is between 1 and 3
    // Deprecated
    void deliverProduct(int slot);

    // Tell PLC, which machine it is
    virtual void identify();
};

}
}
