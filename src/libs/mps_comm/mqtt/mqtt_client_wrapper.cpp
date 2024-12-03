// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  mqtt_client_wrapper.cpp -MQTT client wrapper
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

#include "mqtt_client_wrapper.h"

#if HAVE_SYSTEM_SPDLOG
#	include <spdlog/sinks/basic_file_sink.h>
#	include <spdlog/sinks/stdout_sinks.h>
#endif
#include <spdlog/logger.h>

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

mqtt_client_wrapper::mqtt_client_wrapper(const std::string              &client_id,
                                         std::shared_ptr<spdlog::logger> logger,
                                         std::string                     broker_address_)
{
	logger_               = logger;
	name_                 = client_id;
	command_topic		  = TOPIC_PREFIX + std::string("/") + name_ + "/Command";
	connected             = false;
	broker_address        = broker_address_;
	std::string full_name = "Refbox_" + name_;
	cli                   = new mqtt::async_client(broker_address, full_name);
	subListener_          = new mqtt_action_listener(name_, logger_);
	mqtt::connect_options connOpts;
	connOpts.set_clean_session(true);
	connOpts.set_keep_alive_interval(std::chrono::seconds(1200));
	// Install the callback(s) before connecting.
	//connOpts.set_keep_alive_interval(0);
	callback_handler = new mqtt_callback(*cli, connOpts, logger_);
	cli->set_callback(*callback_handler);
	std::lock_guard<std::mutex> lock(client_mutex);
	// Start the connection.
	// When completed, the callback will subscribe to topic.

	while (!connected) {
		try {
			logger_->info("Connecting to the MQTT server...");
			connected =
			  cli->connect(connOpts, nullptr, *callback_handler)->wait_for(std::chrono::seconds(1));
		} catch (const mqtt::exception &exc) {
			logger_->error("ERROR: Unable to connect to MQTT server: '{}' {}",
			               broker_address,
			               exc.to_string());
			return;
		}
	}
	SubscribeToTopic(TOPIC_PREFIX + std::string("/") + name_ + "/Status");
	SubscribeToTopic(TOPIC_PREFIX + std::string("/") + name_ + "/WP-Sensor");
	SubscribeToTopic(TOPIC_PREFIX + std::string("/") + name_ + "/Barcode");
	if (name_ == "C-RS1" || name_ == "C-RS2" || name_ == "M-RS1" || name_ == "M-RS2") {
		logger_->info("Subscribing to the slidecounter!");
		SubscribeToTopic(TOPIC_PREFIX + std::string("/") + name_ + "/SlideCount");
	}
	connected = true;
}

bool
mqtt_client_wrapper::SetNodeValue(std::string topic, std::string value)
{
	try {
		logger_->info("Setting node value for {} to {}", topic, value);
		mqtt::message_ptr pubmsg = mqtt::make_message(topic, value);
		pubmsg->set_qos(QOS);
		cli->publish(pubmsg)->wait_for(std::chrono::seconds(10));
	} catch (const mqtt::exception &exc) {
		logger_->error("ERROR: Unable to publish to MQTT server: '{}' {}",
		               broker_address,
		               exc.to_string());
		std::this_thread::sleep_for(std::chrono::milliseconds(MQTT_DELAY));
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(MQTT_DELAY));
	return true;
}

void
mqtt_client_wrapper::SubscribeToTopic(std::string topic)
{
	try {
		logger_->info("Subscribing to topic [{}]", topic);
		cli->subscribe(topic, QOS, nullptr, *subListener_)
		  ->wait_for(std::chrono::seconds(5));
	} catch (const mqtt::exception &exc) {
		logger_->error("ERROR: Unable to Subscribe to MQTT topic: '{}' {}", topic, exc.to_string());
		return;
	}
}

mqtt_client_wrapper::~mqtt_client_wrapper()
{
	try {
		logger_->info("Disconnecting from the MQTT server...");
		cli->disconnect()->wait();
		logger_->info("OK");
	} catch (const mqtt::exception &exc) {
		logger_->error("{}", exc.to_string());
		return;
	}
}

bool
mqtt_client_wrapper::dispatch_command(std::string command)
{
	std::lock_guard<std::mutex> lock(client_mutex);
	logger_->info("Starting dispatch of command");
	if (!SetNodeValue(command_topic, command))
		return false;
	logger_->info("Finished dispatch of command successful");
	return true;
}
void
mqtt_client_wrapper::register_busy_callback(std::function<void(bool)> callback)
{
	callback_handler->register_busy_callback(callback);
}

void
mqtt_client_wrapper::register_ready_callback(std::function<void(bool)> callback)
{
	callback_handler->register_ready_callback(callback);
}

void
mqtt_client_wrapper::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	callback_handler->register_barcode_callback(callback);
}
void
mqtt_client_wrapper::register_slide_callback(std::function<void(unsigned int)> callback)
{
	callback_handler->register_slide_callback(callback);
}

} // namespace mps_comm
} // namespace rcll
