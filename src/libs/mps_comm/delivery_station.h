// This file implements the delivery station
#pragma once

#include "machine.h"

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

class DeliveryStation : public Machine
{
public:
	DeliveryStation(std::string name, std::string ip, unsigned short port, ConnectionMode mode);
	virtual ~DeliveryStation();

	// Send command to deliver a product
	// slot is between 1 and 3
	// Deprecated
	void deliver_product(int slot);

	// Tell PLC, which machine it is
	virtual void identify();
};

} // namespace mps_comm
} // namespace llsfrb
