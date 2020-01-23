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
const std::string OpcUaMachine::LOG_PATH =
  ""; /* TODO add log path if needed; if empty -> log is redirected to stdout */

constexpr std::chrono::seconds OpcUaMachine::mock_busy_duration_;
constexpr std::chrono::seconds OpcUaMachine::mock_ready_duration_;

OpcUaMachine::OpcUaMachine(unsigned short int machine_type,
                           std::string        ip,
                           unsigned short     port,
                           ConnectionMode     connection_mode)
: abort_operation_(false),
  machine_type_(machine_type),
  ip_(ip),
  port_(port),
  connection_mode_(connection_mode),
  shutdown_(false),
  heartbeat_active_(false)
{
	std::cout << "OpcUaMachine: name is " << name_ << std::endl;
	initLogger();
	worker_thread_ = std::thread(&OpcUaMachine::dispatch_command_queue, this);
}

void
OpcUaMachine::dispatch_command_queue()
{
	std::unique_lock<std::mutex> lock(command_queue_mutex_);
	// Always process the queue before shutdown
	while (!shutdown_ || !command_queue_.empty()) {
		queue_condition_.wait(lock, [this] { return !command_queue_.empty() || shutdown_; });
		if (!command_queue_.empty()) {
			auto command = command_queue_.front();
			command_queue_.pop();
			lock.unlock();
			command();
			std::this_thread::sleep_for(std::chrono::milliseconds(40));
			lock.lock();
		}
	}
}

void
OpcUaMachine::heartbeat()
{
	heartbeat_active_ = true;
	while (!shutdown_) {
		send_command(COMMAND_NOTHING, 0, 0, 1);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	heartbeat_active_ = false;
}

void
OpcUaMachine::mock_callback(OpcUtils::MPSRegister reg, OpcUtils::ReturnValue *ret)
{
	for (auto &cb : callbacks_) {
		if (std::get<1>(cb) == reg) {
			SubscriptionClient::ReturnValueCallback fct = std::get<0>(cb);
			fct(ret);
		}
	}
}

void
OpcUaMachine::mock_callback(OpcUtils::MPSRegister reg, bool ret)
{
	OpcUtils::ReturnValue ret_val;
	ret_val.bool_s = ret;
	return mock_callback(reg, &ret_val);
}

void
OpcUaMachine::send_command(unsigned short command,
                           unsigned short payload1,
                           unsigned short payload2,
                           int            timeout,
                           unsigned char  status,
                           unsigned char  error)
{
	std::function<void(void)> call = [this, command, payload1, payload2, status, error] {
		std::unique_lock<std::mutex> lock(command_mutex_);
		bool                         success;
		do {
			try {
				if (command > 100) {
					logger->info("Sending command: {} {} {} {}", command, payload1, payload2, status);
				}
				if (connection_mode_ == MOCKUP) {
					if (command > 100) {
						mock_callback(OpcUtils::MPSRegister::STATUS_BUSY_IN, true);
						std::this_thread::sleep_for(mock_busy_duration_);
						mock_callback(OpcUtils::MPSRegister::STATUS_BUSY_IN, false);
						mock_callback(OpcUtils::MPSRegister::STATUS_READY_IN, true);
						std::this_thread::sleep_for(mock_ready_duration_);
						mock_callback(OpcUtils::MPSRegister::STATUS_READY_IN, false);
					}
					return;
				}
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
				success = true;
			} catch (std::exception &e) {
				logger->warn("Failed to send command to {}, reconnecting", name_);
				subscriptions.clear();
				connect_PLC();
				success = false;
			}
		} while (!success);
	};
	std::unique_lock<std::mutex> lock(command_queue_mutex_);
	command_queue_.push(call);
	lock.unlock();
	queue_condition_.notify_one();
	return;
}

void
OpcUaMachine::reset()
{
	send_command(machine_type_ | Command::COMMAND_RESET);
}

bool
OpcUaMachine::connect_PLC()
{
	if (connection_mode_ == MOCKUP) {
		return true;
	}
	bool simulation = (connection_mode_ == SIMULATION);
	if (!reconnect(ip_.c_str(), port_, simulation))
		return false;

	subscribe(SUB_REGISTERS, outs, simulation);
	identify();
	for (auto &cb : callbacks_) {
		register_callback(cb, simulation);
	}
	if (!heartbeat_active_) {
		heartbeat_thread_ = std::thread(&OpcUaMachine::heartbeat, this);
	}
	return true;
}

OpcUaMachine::~OpcUaMachine()
{
	shutdown_ = true;
	queue_condition_.notify_all();
	if (worker_thread_.joinable()) {
		worker_thread_.join();
	}
	if (heartbeat_thread_.joinable()) {
		heartbeat_thread_.join();
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
	send_command(m_color, plc_state, time);
}

void
OpcUaMachine::conveyor_move(ConveyorDirection direction, MPSSensor sensor)
{
	send_command(Command::COMMAND_MOVE_CONVEYOR + machine_type_,
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
OpcUaMachine::initLogger()
{
	if (LOG_PATH.empty() || LOG_PATH.length() < 1) {
		// stdout redirected logging
		logger = spdlog::stdout_logger_mt(name_);
		logger->set_level(spdlog::level::warn);
	} else /* ... or logging to file */ {
		logger = spdlog::basic_logger_mt(name_, LOG_PATH);
	}
	logger->info("\n\n\nNew logging session started");
}

bool
OpcUaMachine::reconnect(const char *ip, unsigned short port, bool simulation)
{
	disconnect();
	try {
		OpcUa::EndpointDescription *endpoint = OpcUtils::getEndpoint(ip, port);
		logger->info("Connecting to: {}", endpoint->EndpointUrl);

		client = new OpcUa::UaClient(logger);
		client->Connect(*endpoint);
		connected_ = true;
	} catch (const std::exception &exc) {
		logger->error("OPC UA connection error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
		return false;
	} catch (...) {
		logger->error("Unknown error.");
		return false;
	}

	try {
		nodeBasic = OpcUtils::getBasicNode(client, simulation);
		nodeIn    = OpcUtils::getInNode(client, simulation);

		for (int i = 0; i < OpcUtils::MPSRegister::STATUS_READY_BASIC; i++)
			registerNodes[i] = OpcUtils::getNode(client, (OpcUtils::MPSRegister)i, simulation);
	} catch (const std::exception &exc) {
		logger->error("Node path error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
		return false;
	} catch (...) {
		logger->error("Unknown error.");
		return false;
	}
	return true;
}

bool
OpcUaMachine::disconnect()
{
	if (!connected_) {
		return true;
	}
	cancelAllSubscriptions(true);

	logger->info("Disconnecting");
	try {
		client->Disconnect();
		logger->flush();
		connected_ = false;
		return true;
	} catch (...) {
		try {
			client->Abort();
			logger->flush();
			connected_ = false;
			return true;
		} catch (...) {
			try {
				delete client;
				client = nullptr;
				logger->flush();
				connected_ = false;
				return true;
			} catch (...) {
			}
		}
	}
	connected_ = false;
	return false;
}

void
OpcUaMachine::subscribeAll(bool simulation)
{
	for (int i = OpcUtils::MPSRegister::ACTION_ID_IN; i != OpcUtils::MPSRegister::STATUS_READY_BASIC;
	     i++)
		subscribe(static_cast<OpcUtils::MPSRegister>(i), nullptr, simulation);
}

void
OpcUaMachine::subscribe(std::vector<OpcUtils::MPSRegister> registers,
                        OpcUtils::ReturnValue *            retVals,
                        bool                               simulation)
{
	for (OpcUtils::MPSRegister reg : registers)
		subscribe(reg, retVals == nullptr ? nullptr : &retVals[reg], simulation);
}

SubscriptionClient *
OpcUaMachine::subscribe(OpcUtils::MPSRegister reg, OpcUtils::ReturnValue *retVal, bool simulation)
{
	auto it = subscriptions.end();
	if ((it = subscriptions.find(reg)) != subscriptions.end())
		return it->second;
	OpcUa::Node         node = OpcUtils::getNode(client, reg, simulation);
	SubscriptionClient *sub  = new SubscriptionClient(logger, retVal);
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
	if (subscriptions.size() == 0)
		return &outs[reg];
	else {
		auto it = subscriptions.find(reg);
		if (it != subscriptions.end())
			return it->second->mpsValue;
	}
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
	for (int i = 0; i < OpcUtils::MPSRegister::STATUS_READY_BASIC; i++)
		OpcUtils::logReturnValue(getReturnValue((OpcUtils::MPSRegister)i),
		                         logger,
		                         (OpcUtils::MPSRegister)i);
}

void
OpcUaMachine::addCallback(SubscriptionClient::ReturnValueCallback callback,
                          OpcUtils::MPSRegister                   reg,
                          OpcUtils::ReturnValue *                 retVal,
                          bool                                    simulation)
{
	Callback cb = std::make_tuple(callback, reg, retVal);
	callbacks_.push_back(cb);
	register_callback(cb);
}

void
OpcUaMachine::register_callback(Callback callback, bool simulation)
{
	if (connection_mode_ == MOCKUP) {
		return;
	}
	logger->info("Registering callback");
	SubscriptionClient *sub = subscribe(std::get<1>(callback), std::get<2>(callback), simulation);
	sub->add_callback(std::get<0>(callback));
}

void
OpcUaMachine::band_on_until_in()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_IN);
}

void
OpcUaMachine::band_on_until_mid()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_MID);
}

void
OpcUaMachine::band_on_until_out()
{
	send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_OUT);
}

} // namespace mps_comm
} // namespace llsfrb
