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

StorageStation::StorageStation(std::string    name,
                               std::string    ip,
                               unsigned short port,
                               ConnectionMode mode)
: Machine(name, Station::STATION_STORAGE, ip, port, mode)
{
}

StorageStation::~StorageStation() {}

void StorageStation::identify() {
  send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_SS);
}

}
}
