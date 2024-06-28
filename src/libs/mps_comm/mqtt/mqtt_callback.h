/***************************************************************************
 *  mqtt_callbacks.h -MQTT communication callbacks
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
#include "mqtt_utils.h"

#include <mqtt/async_client.h>
#include <spdlog/logger.h>

#include <boost/algorithm/string.hpp>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

// when printing recursively, indent is used to make the hirarchy more visible
#define logIndent(i) {i * 2, (i + 1) * 2, (i + 2) * 2}

class mqtt_callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener

{
	// Counter for the number of connection retries
	int                             nretry_;
	std::shared_ptr<spdlog::logger> logger_;
	// The MQTT client
	mqtt::async_client &cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options &connOpts_;
	// An action listener to display the result of actions.
	mqtt_action_listener subListener_;

	// This deomonstrates manually reconnecting to the broker by calling
	// connect() again. This is a possibility for an application that keeps
	// a copy of it's original connect_options, or if the app wants to
	// reconnect with different options.
	// Another way this can be done manually, if using the same options, is
	// to just call the async_client::reconnect() method.
	void reconnect();

	// Re-connection failure
	void on_failure(const mqtt::token &tok) override;

	// (Re)connection success
	// Either this or connected() can be used for callbacks.
	void on_success(const mqtt::token &tok) override;

	// (Re)connection success
	void connected(const std::string &cause) override;
	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string &cause) override;

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override;

	void delivery_complete(mqtt::delivery_token_ptr token) override;

	std::unordered_map<std::string, std::function<void(unsigned int)>> callbacks_;

public:
	void register_busy_callback(std::function<void(bool)> callback);
	void register_ready_callback(std::function<void(bool)> callback);
	void register_barcode_callback(std::function<void(unsigned long)> callback);
	void register_slide_callback(std::function<void(unsigned int)> callback);

	mqtt_callback(mqtt::async_client             &cli,
	              mqtt::connect_options          &connOpts,
	              std::shared_ptr<spdlog::logger> logger);

	//: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {};

	~mqtt_callback();
};

} // namespace mps_comm
} // namespace llsfrb
