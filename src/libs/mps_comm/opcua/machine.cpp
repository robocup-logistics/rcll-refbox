/***************************************************************************
 *  machine.cpp - OPC-UA communication with an MPS
 *
 *  Created: Thu 21 Feb 2019 13:29:11 CET 13:29
 *  Copyright  2019  Alex Maestrini <maestrini@student.tugraz.at>
 *                   Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "machine.h"

#include "../exceptions.h"
#include "../time_utils.h"
#include "mps_io_mapping.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

const std::vector<OpcUtils::MPSRegister>
  OpcUaMachine::SUB_REGISTERS({OpcUtils::MPSRegister::BARCODE_IN,
                               OpcUtils::MPSRegister::ERROR_IN,
                               OpcUtils::MPSRegister::STATUS_BUSY_IN,
                               OpcUtils::MPSRegister::STATUS_ENABLE_IN,
                               OpcUtils::MPSRegister::STATUS_ERROR_IN,
                               OpcUtils::MPSRegister::STATUS_READY_IN});

OpcUaMachine::OpcUaMachine(Station            machine_type,
                           const std::string &ip,
                           unsigned short     port,
                           const std::string &log_path,
                           ConnectionMode     connection_mode)
: machine_type_(machine_type),
  ip_(ip),
  port_(port),
  connection_mode_(connection_mode),
  shutdown_(false),
  connected_(false),
  simulation_(connection_mode == SIMULATION)
{
	initLogger(log_path);
	worker_thread_ = std::thread(&OpcUaMachine::dispatch_command_queue, this);
}

void
OpcUaMachine::dispatch_command_queue()
{
	std::unique_lock<std::mutex> lock(command_queue_mutex_);
	while (!shutdown_) {
		if (!command_queue_.empty()) {
			auto instruction = command_queue_.front();
			command_queue_.pop();
			lock.unlock();
			while (!send_instruction(instruction)) {
				reconnect();
			};
			std::this_thread::sleep_for(std::chrono::milliseconds(40));
			lock.lock();
		} else {
			if (!queue_condition_.wait_for(lock, std::chrono::seconds(1), [&] {
				    return !command_queue_.empty();
			    })) {
				// there was no instruction in the queue, send heartbeat to ensure the
				// connection is healthy and reconnect if it is not
				while (!send_instruction(std::make_tuple(COMMAND_NOTHING, 0, 0, 1, 0, 0))) {
					reconnect();
				}
			}
		}
	}
}

void
OpcUaMachine::enqueue_instruction(unsigned short command,
                                  unsigned short payload1,
                                  unsigned short payload2,
                                  int            timeout,
                                  unsigned char  status,
                                  unsigned char  error)
{
	std::lock_guard<std::mutex> lg(command_queue_mutex_);
	command_queue_.push(std::make_tuple(command, payload1, payload2, timeout, status, error));
}

bool
OpcUaMachine::send_instruction(const Instruction &instruction)

{
	const unsigned short command  = std::get<0>(instruction);
	const unsigned short payload1 = std::get<1>(instruction);
	const unsigned short payload2 = std::get<2>(instruction);
	//const int            timeout  = std::get<3>(instruction);
	const unsigned char status = std::get<4>(instruction);
	const unsigned char error  = std::get<5>(instruction);
	try {
		OpcUtils::MPSRegister registerOffset;
		if (command < Station::STATION_BASE)
			registerOffset = OpcUtils::MPSRegister::ACTION_ID_BASIC;
		else
			registerOffset = OpcUtils::MPSRegister::ACTION_ID_IN;

		bool                  statusBit = (bool)(status & Status::STATUS_BUSY);
		OpcUtils::MPSRegister reg;
		reg = registerOffset + OpcUtils::MPSRegister::ACTION_ID_IN;
		setNodeValue(registerNodes[reg], (uint16_t)command, reg);
		reg = registerOffset + OpcUtils::MPSRegister::DATA_IN;
		setNodeValue(registerNodes[reg].GetChildren()[0], (uint16_t)payload1, reg);
		reg = registerOffset + OpcUtils::MPSRegister::DATA_IN;
		setNodeValue(registerNodes[reg].GetChildren()[1], (uint16_t)payload2, reg);
		reg = registerOffset + OpcUtils::MPSRegister::STATUS_ENABLE_IN;
		setNodeValue(registerNodes[reg], statusBit, reg);
		reg = registerOffset + OpcUtils::MPSRegister::ERROR_IN;
		setNodeValue(registerNodes[reg], (uint8_t)error, reg);
	} catch (std::exception &e) {
		logger->warn("Error while sending command: {}", e.what());
		return false;
	}

	return true;
}

void
OpcUaMachine::reset()
{
	enqueue_instruction(machine_type_ | Command::COMMAND_RESET);
}

void
OpcUaMachine::connect()
{
	if (connection_mode_ == MOCKUP) {
		return;
	}
	while (!reconnect()) {}

	subscribe(SUB_REGISTERS, simulation_);
	identify();
	update_callbacks();
}

OpcUaMachine::~OpcUaMachine()
{
	std::unique_lock<std::mutex> lock(command_queue_mutex_);
	shutdown_ = true;
	lock.unlock();
	queue_condition_.notify_all();
	if (worker_thread_.joinable()) {
		worker_thread_.join();
	}
	disconnect();
}

void
OpcUaMachine::set_light(llsf_msgs::LightColor color,
                        llsf_msgs::LightState state,
                        unsigned short        time)
{
	LightColor m_color;
	switch (color) {
	case llsf_msgs::LightColor::RED: m_color = LightColor::LIGHT_COLOR_RED; break;
	case llsf_msgs::LightColor::YELLOW: m_color = LightColor::LIGHT_COLOR_YELLOW; break;
	case llsf_msgs::LightColor::GREEN: m_color = LightColor::LIGHT_COLOR_GREEN; break;
	}
	switch (m_color) {
	case LightColor::LIGHT_COLOR_RESET:
	case LightColor::LIGHT_COLOR_RED:
	case LightColor::LIGHT_COLOR_YELLOW:
	case LightColor::LIGHT_COLOR_GREEN: break;
	default: throw std::invalid_argument("Illegal color! See MPSIoMapping.h for choices.");
	}
	unsigned short int plc_state;
	switch (state) {
	case llsf_msgs::ON: plc_state = LightState::LIGHT_STATE_ON; break;
	case llsf_msgs::OFF: plc_state = LightState::LIGHT_STATE_OFF; break;
	case llsf_msgs::BLINK: plc_state = LightState::LIGHT_STATE_BLINK; break;
	default:
		plc_state = LightState::LIGHT_STATE_OFF;
		// TODO error
	}
	enqueue_instruction(m_color, plc_state, time);
}

void
OpcUaMachine::conveyor_move(ConveyorDirection direction, MPSSensor sensor)
{
	enqueue_instruction(Command::COMMAND_MOVE_CONVEYOR + machine_type_,
	                    sensor,
	                    direction,
	                    Timeout::TIMEOUT_BAND);
}

void
OpcUaMachine::reset_light()
{
	set_light(llsf_msgs::LightColor::RED, llsf_msgs::OFF);
}

void
OpcUaMachine::initLogger(const std::string &log_path)
{
	if (log_path.empty()) {
		// stdout redirected logging
		logger = spdlog::stdout_logger_mt(name_);
		logger->set_level(spdlog::level::warn);
	} else /* ... or logging to file */ {
		logger = spdlog::basic_logger_mt(name_, log_path);
		logger->set_level(spdlog::level::info);
	}
	logger->info("\n\n\nNew logging session started");
}

bool
OpcUaMachine::reconnect()
{
	disconnect();
	try {
		OpcUa::EndpointDescription endpoint = OpcUtils::getEndpoint(ip_.c_str(), port_);
		logger->info("Connecting to: {}", endpoint.EndpointUrl);

		client = std::make_unique<OpcUa::UaClient>(logger);
		client->Connect(endpoint);
		connected_ = true;
	} catch (const std::exception &exc) {
		logger->error("OPC UA connection error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
		return false;
	} catch (...) {
		logger->error("Unknown error.");
		return false;
	}

	try {
		nodeBasic = OpcUtils::getBasicNode(client.get(), simulation_);
		nodeIn    = OpcUtils::getInNode(client.get(), simulation_);

		for (int i = 0; i < OpcUtils::MPSRegister::LAST; i++)
			registerNodes[i] = OpcUtils::getNode(client.get(), (OpcUtils::MPSRegister)i, simulation_);
	} catch (const std::exception &exc) {
		logger->error("Node path error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
		return false;
	} catch (...) {
		logger->error("Unknown error.");
		return false;
	}
	return true;
}

void
OpcUaMachine::disconnect()
{
	if (!connected_) {
		return;
	}
	try {
		cancelAllSubscriptions(true);
	} catch (std::exception &e) {
		logger->warn("Error while cancelling subscriptions: {}", e.what());
	}
	subscriptions.clear();

	logger->info("Disconnecting");
	try {
		client->Disconnect();
		logger->flush();
		connected_ = false;
		return;
	} catch (...) {
		try {
			client->Abort();
			logger->flush();
			connected_ = false;
			return;
		} catch (...) {
			try {
				logger->flush();
				connected_ = false;
				return;
			} catch (...) {
			}
		}
	}
	connected_ = false;
	return;
}

void
OpcUaMachine::subscribeAll(bool simulation)
{
	for (int i = OpcUtils::MPSRegister::ACTION_ID_IN; i < OpcUtils::MPSRegister::LAST; i++)
		subscribe(static_cast<OpcUtils::MPSRegister>(i), simulation);
}

void
OpcUaMachine::subscribe(std::vector<OpcUtils::MPSRegister> registers, bool simulation)
{
	for (OpcUtils::MPSRegister reg : registers)
		subscribe(reg, simulation);
}

SubscriptionClient *
OpcUaMachine::subscribe(OpcUtils::MPSRegister reg, bool simulation)
{
	auto it = subscriptions.end();
	if ((it = subscriptions.find(reg)) != subscriptions.end())
		return it->second;
	OpcUa::Node         node = OpcUtils::getNode(client.get(), reg, simulation);
	SubscriptionClient *sub  = new SubscriptionClient(logger);
	sub->reg                 = reg;
	sub->node                = node;

	int response_timeout = 100;
	sub->subscription    = client->CreateSubscription(response_timeout, *sub);
	sub->handle          = sub->subscription->SubscribeDataChange(node);
	logger->info("Subscribed to {} (name: {}, handle: {})",
	             OpcUtils::REGISTER_NAMES[reg],
	             node.GetBrowseName().Name,
	             sub->handle);
	subscriptions.insert(SubscriptionClient::pair(reg, sub));
	return sub;
}

void
OpcUaMachine::cancelAllSubscriptions(bool log)
{
	if (log)
		printFinalSubscribtions();

	for (SubscriptionClient::map::iterator it = subscriptions.begin(); it != subscriptions.end();) {
		OpcUtils::MPSRegister reg = it->first;
		SubscriptionClient *  sub = it->second;
		sub->subscription->UnSubscribe(sub->handle);
		logger->info("Unsubscribed from {} (name: {}, handle: {})",
		             OpcUtils::REGISTER_NAMES[reg],
		             sub->node.GetBrowseName().Name,
		             sub->handle);
		it = subscriptions.erase(it);
	}
}

SubscriptionClient::map::iterator
OpcUaMachine::cancelSubscription(OpcUtils::MPSRegister reg, bool log)
{
	auto it = subscriptions.find(reg);
	if (it != subscriptions.end()) {
		SubscriptionClient *sub = it->second;
		sub->subscription->UnSubscribe(sub->handle);
		logger->info("Unsubscribed from {} (name: {}, handle: {})",
		             OpcUtils::REGISTER_NAMES[reg],
		             sub->node.GetBrowseName().Name,
		             sub->handle);
		if (log)
			OpcUtils::logReturnValue(getReturnValue(reg), logger, reg);
		return subscriptions.erase(it);
	}
	return it;
}

OpcUtils::ReturnValue *
OpcUaMachine::getReturnValue(OpcUtils::MPSRegister reg)
{
	auto it = subscriptions.find(reg);
	if (it != subscriptions.end())
		return it->second->mpsValue;
	return nullptr;
}

bool
OpcUaMachine::setNodeValue(OpcUa::Node node, boost::any val, OpcUtils::MPSRegister reg)
{
	SubscriptionClient::map::iterator it = subscriptions.find(reg);
	if (it != subscriptions.end())
		return OpcUtils::setNodeValue(node, val, it->second->mpsValue);
	return OpcUtils::setNodeValue(node, val);
}

void
OpcUaMachine::printFinalSubscribtions()
{
	if (subscriptions.size() > 0)
		logger->info("Final values of subscribed registers:");
	for (int i = 0; i < OpcUtils::MPSRegister::LAST; i++)
		OpcUtils::logReturnValue(getReturnValue((OpcUtils::MPSRegister)i),
		                         logger,
		                         (OpcUtils::MPSRegister)i);
}

void
OpcUaMachine::register_opc_callback(SubscriptionClient::ReturnValueCallback callback,
                                    OpcUtils::MPSRegister                   reg)
{
	logger->info("Registering callback");
	SubscriptionClient *sub = subscribe(reg, simulation_);
	sub->add_callback(callback);
}

void
OpcUaMachine::update_callbacks()
{
	if (!connected_) {
		return;
	}
	for (const auto &cb : callbacks_) {
		register_opc_callback(cb.second, cb.first);
	}
}

void
OpcUaMachine::register_busy_callback(std::function<void(bool)> callback)
{
	if (callback) {
		callbacks_[OpcUtils::MPSRegister::STATUS_BUSY_IN] = [=](OpcUtils::ReturnValue *ret) {
			callback(ret->bool_s);
		};
	} else {
		callbacks_.erase(OpcUtils::MPSRegister::STATUS_BUSY_IN);
	}
	update_callbacks();
}

void
OpcUaMachine::register_ready_callback(std::function<void(bool)> callback)
{
	if (callback) {
		callbacks_[OpcUtils::MPSRegister::STATUS_READY_IN] = [=](OpcUtils::ReturnValue *ret) {
			callback(ret->bool_s);
		};
	} else {
		callbacks_.erase(OpcUtils::MPSRegister::STATUS_READY_IN);
	}
	update_callbacks();
}

void
OpcUaMachine::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	if (callback) {
		callbacks_[OpcUtils::MPSRegister::BARCODE_IN] = [=](OpcUtils::ReturnValue *ret) {
			callback(ret->bool_s);
		};
	} else {
		callbacks_.erase(OpcUtils::MPSRegister::BARCODE_IN);
	}
	update_callbacks();
}

void
OpcUaMachine::identify()
{
	send_instruction(
	  std::make_tuple(Command::COMMAND_SET_TYPE, get_type_from_station(machine_type_), 0, 0, 1, 0));
}

} // namespace mps_comm
} // namespace llsfrb
