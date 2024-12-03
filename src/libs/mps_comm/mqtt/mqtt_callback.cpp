// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
*  mqtt_callbacks.cpp -MQTT communication callbacks
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

#include "mqtt_callback.h"

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

mqtt_callback::mqtt_callback(mqtt::async_client             &cli,
                             mqtt::connect_options          &connOpts,
                             std::shared_ptr<spdlog::logger> logger)
: nretry_(0), logger_(logger), cli_(cli), connOpts_(connOpts), subListener_("Subscription", logger)
{
}

mqtt_callback::~mqtt_callback()
{
}

void
mqtt_callback::reconnect()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	try {
		cli_.connect(connOpts_, nullptr, *this);
	} catch (const mqtt::exception &exec) {
		logger_->error("Error: {}", exec.what());
		exit(1);
	}
}

void
mqtt_callback::on_failure(const mqtt::token &tok)
{
	logger_->info("Connection attempt failed");
	if (++nretry_ > MQTT_RETRY_COUNT)
		exit(1);
	reconnect();
}

void
mqtt_callback::on_success(const mqtt::token &tok)
{
}

void
mqtt_callback::connected(const std::string &cause)
{
	logger_->info("Connection success");
}

void
mqtt_callback::connection_lost(const std::string &cause)
{
	if (cause.empty()) {
		logger_->info("Connection lost with unknown reason!");
	} else {
		logger_->info("Connection lost with cause: {}!", cause);
	}
	logger_->info("Reconnecting...");
	nretry_ = 0;
	reconnect();
}

void
mqtt_callback::message_arrived(mqtt::const_message_ptr msg)
{
	std::string topic = msg->get_topic();
	std::string value = msg->to_string();

	size_t pos = topic.find_last_of('/');
    // Extract the substring after the last '/'
    std::string topic_name = topic.substr(pos + 1);

	if(topic_name == "Status") {
		logger_->info("Received an Status with value " + value);
		bool busy = false;
		if (value == "IDLE") {
			busy = false;
		} else if(value == "BUSY") {
			busy = true;
		} else {
			logger_->error("Unknown status value");
		}

		if (callback_busy)
			std::thread([this, busy]() {
				callback_busy(busy);
			}).detach();

	} else if (topic_name == "WP-Sensor") {
		logger_->info("MPS sent a WP-Sensor update with value [{}]", value);
		bool ready = false;
		if (value == "WP") {
			ready = true;
		} else if (value == "NoWP") {
			ready = false;
		} else {
			logger_->error("Unknown WP-Sensor value");
		}

		if (callback_ready)
			std::thread([this, ready]() {
				callback_ready(ready);
			}).detach();

	} else if(topic_name == "SlideCount"){
		logger_->info("MPS sent a InSlideCnt update with value [{}]", value);
		unsigned int count = std::stoul(value);
		if (callback_slide)
			std::thread([this, count]() {
				callback_slide(count);
			}).detach();
	}
	else if(topic_name == "Barcode") {
		logger_->info("MPS sent a Barcode update with value [{}]", value);
		if(callback_barcode)
			callback_barcode(std::stoul(value));
	} else {
		logger_->info("Not a known topic");
	}
}

void
mqtt_callback::delivery_complete(mqtt::delivery_token_ptr token)
{
}

void
mqtt_callback::register_ready_callback(std::function<void(bool)> callback)
{
	callback_ready = callback;
}

void
mqtt_callback::register_busy_callback(std::function<void(bool)> callback)
{
	callback_busy = callback;
}

void
mqtt_callback::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	callback_barcode = callback;
}

void
mqtt_callback::register_slide_callback(std::function<void(unsigned int)> callback)
{
	callback_slide = callback;
}

} // namespace mps_comm
} // namespace rcll
