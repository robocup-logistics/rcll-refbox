// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
*  mqtt_callbacks.cpp -MQTT_LEGACY communication callbacks
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

mqtt_legacy_callback::mqtt_legacy_callback(mqtt::async_client             &cli,
                             mqtt::connect_options          &connOpts,
                             std::shared_ptr<spdlog::logger> logger)
: nretry_(0), logger_(logger), cli_(cli), connOpts_(connOpts), subListener_("Subscription", logger)
{
}

mqtt_legacy_callback::~mqtt_legacy_callback()
{
}

void
mqtt_legacy_callback::reconnect()
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
mqtt_legacy_callback::on_failure(const mqtt::token &tok)
{
	logger_->info("Connection attempt failed");
	if (++nretry_ > MqttLegacyUtils::N_RETRY_ATTEMPTS)
		exit(1);
	reconnect();
}

void
mqtt_legacy_callback::on_success(const mqtt::token &tok)
{
}

void
mqtt_legacy_callback::connected(const std::string &cause)
{
	logger_->info("Connection success");
}

void
mqtt_legacy_callback::connection_lost(const std::string &cause)
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
mqtt_legacy_callback::message_arrived(mqtt::const_message_ptr msg)
{
	//std::cout << "Message arrived" << std::endl;
	//std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
	//std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;

	std::string topic = msg->get_topic();
	std::string value = msg->to_string();
	switch (MqttLegacyUtils::ParseTopic(topic)) {
	case Topic::BasicNodes_Busy:
		//std::cout << "MPS sent a BasicBusy update with value " << value << std::endl;
		logger_->info("Received an updated for Basic/Busy with value " + value);
		break;
	case Topic::BasicNodes_Ready:
		//std::cout << "MPS sent a BasicEnabled update with value " << value << std::endl;
		logger_->info("Received an updated for Basic/Ready with value " + value);
		break;
	case Topic::InNodes_Busy: {
		//std::cout << "MPS sent a InBusy update with value [" << value << "]" << std::endl;
		logger_->info("Received an updated for In/Busy with value " + value);
		bool val = false;
		if (boost::algorithm::to_lower_copy(value).compare("true") == 0) {
			val = true;
		}
		if (callbacks_[MqttLegacyUtils::bits[0]]) {
			callbacks_[MqttLegacyUtils::bits[0]](val);
		} else {
			logger_->info("Received something but no callback registered ");
		}
		break;
	}
	case Topic::InNodes_Ready: {
		//std::cout << "MPS sent a InEnabled update with value [" << value << "]" << std::endl;
		logger_->info("Received an updated for In/Ready with value " + value);
		bool val = false;
		if (boost::algorithm::to_lower_copy(value).compare("true") == 0) {
			val = true;
		}
		if (callbacks_[MqttLegacyUtils::bits[1]]) {
			callbacks_[MqttLegacyUtils::bits[1]](val);
		} else {
			logger_->info("Received something but no callback registered ");
		}
		break;
	}
	case Topic::InNodes_Sldcnt: {
		logger_->info("MPS sent a InSlideCnt update with value [{}]", value);
		unsigned int count = std::stoul(value);
		if (callbacks_[MqttLegacyUtils::registers[5]]) {
			callbacks_[MqttLegacyUtils::registers[5]](count);
		} else {
			logger_->info("Received something but no callback registered ");
		}
		// TODO maybe fix and use this instead with std::any in the map std::any_cast <int (*) (int)> (mapIter->second) (5)
		break;
	}
	default: logger_->info("Not a known topic");
	}
}

void
mqtt_legacy_callback::delivery_complete(mqtt::delivery_token_ptr token)
{
}

void
mqtt_legacy_callback::register_busy_callback(std::function<void(bool)> callback)
{
	callbacks_[MqttLegacyUtils::bits[0]] = callback;
}

void
mqtt_legacy_callback::register_ready_callback(std::function<void(bool)> callback)
{
	callbacks_[MqttLegacyUtils::bits[1]] = callback;
}

void
mqtt_legacy_callback::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	callbacks_[MqttLegacyUtils::registers[1]] = callback;
}
void
mqtt_legacy_callback::register_slide_callback(std::function<void(unsigned int)> callback)
{
	callbacks_[MqttLegacyUtils::registers[5]] = callback;
}

} // namespace mps_comm
} // namespace rcll
