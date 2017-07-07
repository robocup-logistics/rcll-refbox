#include "DeliveryStation.h"
#include "MPSIoMapping.h"
#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

DeliveryStation::DeliveryStation() : Machine(DELIVERY_STATION_CMD) { }
DeliveryStation::~DeliveryStation() {}

void DeliveryStation::deliverProduct(int slot) {
  //lock_guard<mutex> g(lock_);
  sendCommand(machine_type_ | DELIVER_CMD, slot);
  waitForReady();
}

void DeliveryStation::identify() {
  sendCommand(SET_TYPE_CMD, TYPE_DS);
}

}
}
