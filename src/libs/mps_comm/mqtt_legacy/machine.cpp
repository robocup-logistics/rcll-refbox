// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  machine.cpp - MQTT_LEGACY communication with an MPS
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

#include "./machine.h"

#include "../exceptions.h"
#include "../time_utils.h"

#if HAVE_SYSTEM_SPDLOG
#	include <spdlog/sinks/basic_file_sink.h>
#	include <spdlog/sinks/stdout_sinks.h>
#endif

#include <chrono>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif
using Instruction =
  std::tuple<unsigned short, unsigned short, unsigned short, int, unsigned char, unsigned char>;

MqttLegacyMachine::MqttLegacyMachine(const std::string &name,
                         Station            machine_type,
                         const std::string &ip,
                         unsigned short     port,
                         const std::string &log_path,
                         ConnectionMode     connection_mode)
: connected_(false),
  client_id_(name),
  machine_type_(machine_type),
  ip_(ip),
  port_(port),
  connection_mode_(connection_mode),
  shutdown_(false),
  simulation_(connection_mode == SIMULATION)
{
	initLogger(log_path);
	auto broker = std::string("tcp://" + ip_ + ":" + std::to_string(port_));
	logger->info("Connecting to {}", broker);
	mqtt_legacy_client_               = new mqtt_legacy_client_wrapper(client_id_, logger, broker);
	subscribed_                = false;
	start_sending_instructions = false;
	running                    = true;

	//worker_thread_ = std::thread(&MqttLegacyMachine::client_keep_alive, this);
	dispatcher_thread_ = std::thread(&MqttLegacyMachine::dispatch_command_queue, this);
}

void
MqttLegacyMachine::dispatch_command_queue()
{
	while (running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if (!mqtt_legacy_client_->connected) {
			continue;
		}
		command_queue_mutex_.lock();
		if (command_queue_.empty()) {
			command_queue_mutex_.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(180));
			continue;
		}
		if (mqtt_legacy_client_->dispatch_command(command_queue_.front())) {
			command_queue_.pop();
		}
		command_queue_mutex_.unlock();
	}
}

void
MqttLegacyMachine::enqueue_instruction(unsigned short command,
                                 unsigned short payload1,
                                 unsigned short payload2,
                                 int            timeout,
                                 unsigned char  status,
                                 unsigned char  error)
{
	std::lock_guard<std::mutex> lock(command_queue_mutex_);
	command_queue_.push(std::make_tuple(command, payload1, payload2, timeout, status, error));
	//logger->info("Enqueued a instruction {} {} {} {} {} {}", command, payload1, payload2, timeout, status, error);
}

void
MqttLegacyMachine::reset()
{
	enqueue_instruction(Command::COMMAND_SET_TYPE, machine_type_ / 100);
}

MqttLegacyMachine::~MqttLegacyMachine()
{
	delete mqtt_legacy_client_;
	spdlog::drop(name_);
}

void
MqttLegacyMachine::set_light(llsf_msgs::LightColor color,
                       llsf_msgs::LightState state,
                       unsigned short        time)
{
	LightColor m_color = LIGHT_COLOR_RESET;
	switch (color) {
	case llsf_msgs::LightColor::RED: m_color = LightColor::LIGHT_COLOR_RED; break;
	case llsf_msgs::LightColor::YELLOW: m_color = LightColor::LIGHT_COLOR_YELLOW; break;
	case llsf_msgs::LightColor::GREEN: m_color = LightColor::LIGHT_COLOR_GREEN; break;
	}
	unsigned short int plc_state = LightState::LIGHT_STATE_OFF;
	switch (state) {
	case llsf_msgs::ON: plc_state = LightState::LIGHT_STATE_ON; break;
	case llsf_msgs::OFF: plc_state = LightState::LIGHT_STATE_OFF; break;
	case llsf_msgs::BLINK: plc_state = LightState::LIGHT_STATE_BLINK; break;
	}
	enqueue_instruction(m_color, plc_state, time);
}

void
MqttLegacyMachine::conveyor_move(ConveyorDirection direction, MPSSensor sensor)
{
	enqueue_instruction(Command::COMMAND_MOVE_CONVEYOR + machine_type_,
	                    sensor,
	                    direction,
	                    Timeout::TIMEOUT_BAND);
}

void
MqttLegacyMachine::reset_light()
{
	set_light(llsf_msgs::LightColor::RED, llsf_msgs::OFF);
}

void
MqttLegacyMachine::initLogger(const std::string &log_path)
{
	if (log_path.empty()) {
		// stdout redirected logging
		logger = spdlog::stdout_logger_mt(name_);
		logger->set_level(spdlog::level::info);
	} else /* ... or logging to file */ {
		logger = spdlog::basic_logger_mt(name_, log_path);
		logger->flush_on(spdlog::level::info);
		logger->set_level(spdlog::level::info);
	}
	logger->info("\n\n\nNew logging session started");
}

void
MqttLegacyMachine::register_busy_callback(std::function<void(bool)> callback)
{
	mqtt_legacy_client_->register_busy_callback(callback);
}

void
MqttLegacyMachine::register_ready_callback(std::function<void(bool)> callback)
{
	mqtt_legacy_client_->register_ready_callback(callback);
}

void
MqttLegacyMachine::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	mqtt_legacy_client_->register_barcode_callback(callback);
}

void
MqttLegacyMachine::identify()
{
}

} // namespace mps_comm
} // namespace rcll
