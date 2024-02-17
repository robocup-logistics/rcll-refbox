/***************************************************************************
 *  machine.h - MQTT communication with an MPS
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

// Abstract base class for the stations
#pragma once

#include "../machine.h"
#include "../mps_io_mapping.h"
#include "mqtt_action_listener.h"
#include "mqtt_callback.h"
#include "mqtt_client_wrapper.h"
#include "mqtt_utils.h"

#include <spdlog/spdlog.h>

#include <boost/any.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace llsfrb {
namespace mps_comm {

class MachineFactory;

class MqttMachine : public virtual Machine
{
	using Instruction =
	  std::tuple<unsigned short, unsigned short, unsigned short, int, unsigned char, unsigned char>;

	friend class MachineFactory;

public:
	MqttMachine(const std::string &name,
	            Station            machine_type,
	            const std::string &ip,
	            unsigned short     port,
	            const std::string &log_path = "",
	            ConnectionMode              = MQTT);

	~MqttMachine() override;

	// Set the light of specified color to specified state
	// color: 1 - 3, state 0 - 2
	void set_light(llsf_msgs::LightColor color,
	               llsf_msgs::LightState state = llsf_msgs::ON,
	               unsigned short        time  = 0) override;

	void conveyor_move(ConveyorDirection direction, MPSSensor sensor) override;

	// Sends reset light command
	void reset_light() override;

	// Reset: send the reset command (which is different for each machine type)
	void reset() override;
	void register_busy_callback(std::function<void(bool)> callback) override;
	void register_ready_callback(std::function<void(bool)> callback) override;
	void register_barcode_callback(std::function<void(unsigned long)> callback) override;
	// Identify: The PLC does not know, which machine it runs. This command tells it the type.
	virtual void identify();

	std::shared_ptr<spdlog::logger> logger;
	std::atomic<bool>               start_sending_instructions;
	std::atomic<bool>               connected_;
	const std::string              &client_id_;
	const Station                   machine_type_;
	std::mutex                      command_queue_mutex_;
	std::mutex                      client_mutex_;
	mqtt_client_wrapper            *mqtt_client_;

protected:
	void enqueue_instruction(unsigned short command,
	                         unsigned short payload1 = 0,
	                         unsigned short payload2 = 0,
	                         int            timeout  = 0,
	                         unsigned char  status   = 1,
	                         unsigned char  error    = 0);
	void dispatch_command_queue();

	// Initialize logger; If log_path is empty, the logs are redirected to
	// std::cout, else they are saved to the in log_path specified file
	void initLogger(const std::string &log_path);
	// Helper function to set OPC UA Node value correctly

	const std::string ip_;
	unsigned short    port_;

	const ConnectionMode connection_mode_;

	bool shutdown_;

	std::mutex              command_mutex_;
	std::condition_variable queue_condition_;
	std::queue<Instruction> command_queue_;
	std::thread             worker_thread_;
	std::thread             dispatcher_thread_;

	bool simulation_;
	bool subscribed_;
	bool running;
};

} // namespace mps_comm
} // namespace llsfrb
