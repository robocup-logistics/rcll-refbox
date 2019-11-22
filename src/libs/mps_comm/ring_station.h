// The ring station mounts rings on bases
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

class RingStation : public Machine
{
	static const std::vector<OpcUtils::MPSRegister> SUB_REGISTERS;

public:
	RingStation(std::string name, std::string ip, unsigned short port, ConnectionMode mode);
	virtual ~RingStation();

	// Send command to get a ring
	// void getRing();
	// Check, if the cap is ready for take away
	// deprecated
	void band_on_until_in();
	void band_on_until_mid();
	void band_on_until_out();
	bool ring_ready();

	void mount_ring(unsigned int feeder);

	// identify: tell PLC, which machine it is running on
	virtual void identify();
};

} // namespace mps_comm
} // namespace llsfrb
