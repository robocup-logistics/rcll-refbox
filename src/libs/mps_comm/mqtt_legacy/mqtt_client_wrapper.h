/***************************************************************************
 *  mqtt_client_wrapper.h -MQTT_LEGACY client wrapper
 *
 *  Created: Thu 21 Feb 2023 13:29:11 CET 13:29
 *  Copyright  2023  Dominik Lampel <lampel@student.tugraz.at>
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#pragma once
#include "mqtt_action_listener.h"
#include "mqtt_callback.h"
#include "mqtt_utils.h"

#include <mqtt/async_client.h>

#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>

#if HAVE_SYSTEM_SPDLOG
#	include <spdlog/sinks/basic_file_sink.h>
#	include <spdlog/sinks/stdout_sinks.h>
#endif

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

// when printing recursively, indent is used to make the hirarchy more visible
#define logIndent(i) {i * 2, (i + 1) * 2, (i + 2) * 2}

using Instruction =
  std::tuple<unsigned short, unsigned short, unsigned short, int, unsigned char, unsigned char>;

class mqtt_legacy_client_wrapper
{
	std::string                     name_;
	mqtt::async_client             *cli;
	mqtt_legacy_action_listener           *subListener_;
	mqtt_legacy_callback                  *callback_handler;
	std::shared_ptr<spdlog::logger> logger_;
	std::string                     broker_address;
	std::mutex                      client_mutex;

public:
	std::atomic<bool> connected;
	mqtt_legacy_client_wrapper(const std::string              &client_id,
	                    std::shared_ptr<spdlog::logger> logger,
	                    std::string                     broker_address_);
	~mqtt_legacy_client_wrapper();
	bool SetNodeValue(std::string topic, std::string value);
	void SubscribeToTopic(std::string topic);
	bool dispatch_command(Instruction command);
	void register_busy_callback(std::function<void(bool)> callback);
	void register_ready_callback(std::function<void(bool)> callback);
	void register_barcode_callback(std::function<void(unsigned long)> callback);
	void register_slide_callback(std::function<void(unsigned int)>);
};

} // namespace mps_comm
} // namespace rcll
