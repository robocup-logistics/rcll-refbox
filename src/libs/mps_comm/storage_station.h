// Storage Station
// The storage station can be used to store products.
// It will not work as intended, because protocols do not match yet.
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

class StorageStation: public Machine {
  public:
    StorageStation(std::string name, std::string ip, unsigned short port);
    virtual ~StorageStation();

    // Send command to deliver a product
    // slot is between 1 and 3
    // deprecated
    void get_product(int slot);

    // identify: tell the PLC, which machine it is controlling
    virtual void identify();
};

}
}
