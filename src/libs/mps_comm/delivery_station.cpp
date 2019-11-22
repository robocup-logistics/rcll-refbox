#include "delivery_station.h"

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

DeliveryStation::DeliveryStation(std::string    name,
                                 std::string    ip,
                                 unsigned short port,
                                 ConnectionMode mode)
: Machine(name, Station::STATION_DELIVERY, ip, port, mode)
{
}

DeliveryStation::~DeliveryStation()
{
}

void
DeliveryStation::deliver_product(int slot)
{
	send_command(machine_type_ | Operation::OPERATION_DELIVER, slot);
}

void
DeliveryStation::identify()
{
	send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_DS);
}

} // namespace mps_comm
} // namespace llsfrb
