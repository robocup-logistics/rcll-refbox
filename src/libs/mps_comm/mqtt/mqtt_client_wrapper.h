#pragma once
#include <mqtt/async_client.h>
#include "mqtt_utils.h"
#include "mqtt_callback.h"
#include "mqtt_action_listener.h"
#include <chrono>
#include <thread>
#include <atomic>

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

using Instruction =
	  std::tuple<unsigned short, unsigned short, unsigned short, int, unsigned char, unsigned char>;

class mqtt_client_wrapper
{
	std::string name_;
    mqtt::async_client *cli;
    mqtt_action_listener *subListener_;
	mqtt_callback* callback_handler;
public:
	std::atomic<bool> connected;
	mqtt_client_wrapper(const std::string& client_id);
	~mqtt_client_wrapper();
    bool SetNodeValue(std::string topic, std::string value);
    void SubscribeToTopic(std::string topic);
	bool dispatch_command(Instruction command);
	void register_busy_callback(std::function<void(bool)> callback);
	void register_ready_callback(std::function<void(bool)> callback);
	void register_barcode_callback(std::function<void(unsigned long)> callback);
	void register_slide_callback(std::function<void(unsigned int)>);
};


} // namespace mps_comm
} // namespace llsfrb