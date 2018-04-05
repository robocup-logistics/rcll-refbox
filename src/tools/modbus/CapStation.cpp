#include "CapStation.h"
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

CapStation::CapStation() : Machine(Station::STATION_CAP) { }
CapStation::~CapStation() {}

void CapStation::retrieve_cap() {
  send_command(Operation::OPERATION_CAP_ACTION + machine_type_, Operation::OPERATION_CAP_RETRIEVE);
}

void CapStation::mount_cap() {
  send_command(Operation::OPERATION_CAP_ACTION + machine_type_, Operation::OPERATION_CAP_MOUNT);
}

void CapStation::identify() {
  send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_CS);
}

}
}
