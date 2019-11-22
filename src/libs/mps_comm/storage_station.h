// Storage Station
// The storage station can be used to store products.
// It will not work as intended, because protocols do not match yet.
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

class StorageStation : public Machine
{
public:
	StorageStation(std::string name, std::string ip, unsigned short port, ConnectionMode mode);
	virtual ~StorageStation();

	// identify: tell the PLC, which machine it is controlling
	virtual void identify();
};

} // namespace mps_comm
} // namespace llsfrb
