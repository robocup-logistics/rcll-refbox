#pragma once
#include <mqtt/async_client.h>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

// when printing recursively, indent is used to make the hirarchy more visible
#define logIndent(i)                \
	{                                 \
		i * 2, (i + 1) * 2, (i + 2) * 2 \
	}


class mqtt_action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;
	void on_failure(const mqtt::token& tok) override;
	void on_success(const mqtt::token& tok) override;

public:
	mqtt_action_listener(const std::string& name);
	~mqtt_action_listener();
};


} // namespace mps_comm
} // namespace llsfrb