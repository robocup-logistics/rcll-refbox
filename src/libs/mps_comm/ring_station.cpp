#include "ring_station.h"
#include "mps_io_mapping.h"
#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

const std::vector<OpcUtils::MPSRegister> RingStation::SUB_REGISTERS({ OpcUtils::MPSRegister::SLIDECOUNT_IN, OpcUtils::MPSRegister::BARCODE_IN, OpcUtils::MPSRegister::ERROR_IN, OpcUtils::MPSRegister::STATUS_BUSY_IN, OpcUtils::MPSRegister::STATUS_ENABLE_IN, OpcUtils::MPSRegister::STATUS_ERROR_IN, OpcUtils::MPSRegister::STATUS_READY_IN });

RingStation::RingStation() : Machine(Station::STATION_RING) { }
RingStation::~RingStation() {}

/*void RingStation::getRing() {
  lock_guard<mutex> g(lock_);
  sendCommand(RING_STATION_CMD | GET_RING_CMD);
  waitForReady();
}*/

// Need information on how to access this
bool RingStation::ring_ready() {
  std::cout << "Not implemented yet!" << std::endl;
  return true;
}

void RingStation::identify() {
  send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_RS);
}

void RingStation::mount_ring(unsigned int feeder) {
  send_command(Operation::OPERATION_MOUNT_RING + machine_type_, feeder + 1);
}

}
}
