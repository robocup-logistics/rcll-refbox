#include "storage_station.h"
#include "mps_io_mapping.h"
#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

StorageStation::StorageStation(std::string name, std::string ip, unsigned short port) : Machine(name, Station::STATION_STORAGE, ip, port) { }
StorageStation::~StorageStation() {}

void StorageStation::get_product(int slot) {
  //lock_guard<mutex> g(lock_);
  send_command(machine_type_ + Operation::OPERATION_GET_F_PRODUCT, slot);
  wait_for_ready();
}

void StorageStation::identify() {
  send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_SS);
}

}
}
