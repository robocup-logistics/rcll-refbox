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
#include <open62541.h>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif
inline const std::chrono::milliseconds opcua_poll_rate_{40};

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
	if(!simulation_)
	{
		client = UA_Client_new();
		std::cout << "Creating new Client!" << std::endl;
	}

	worker_thread_ = std::thread(&OpcUaMachine::dispatch_command_queue, this);
}

void
OpcUaMachine::dispatch_command_queue()
{
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
	std::unique_lock<std::mutex> lock(command_queue_mutex_);
	while (!shutdown_) {
		if (!command_queue_.empty()) {
			std::cout << "Non Empty Queue?!" << std::endl;
			auto instruction = command_queue_.front();
			command_queue_.pop();
			lock.unlock();
			while (!send_instruction(instruction)) {
				std::cout << "Reconnect?!" << std::endl;
				reconnect();
			};
			lock.lock();
		} else {
			std::cout << "Empty Queue?!" << std::endl;
			if (!queue_condition_.wait_for(lock, std::chrono::seconds(1), [&] {
				    return !command_queue_.empty();
			    })) {
				// there was no instruction in the queue, send heartbeat to ensure the
				// connection is healthy and reconnect if it is not
				lock.unlock();
				while (!send_instruction(std::make_tuple(COMMAND_NOTHING, 0, 0, 1, 0, 0))) {
					reconnect();
				}
				lock.lock();
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
	const int            timeout  = std::get<3>(instruction);
	const unsigned char  status   = std::get<4>(instruction);
	const unsigned char  error    = std::get<5>(instruction);
	std::cout << "Sending instruction " << command << " " << payload1 << " " << payload2 << " " << timeout << " " <<status <<" "<< error << std::endl;
	logger->info(
	  "Sending instruction {} {} {} {} {} {}", command, payload1, payload2, timeout, status, error);
	try {
		OpcUtils::MPSRegister registerOffset;
		if (command < Station::STATION_BASE)
			registerOffset = OpcUtils::MPSRegister::ACTION_ID_BASIC;
		else
			registerOffset = OpcUtils::MPSRegister::ACTION_ID_IN;

		bool                  statusBit = (bool)(status & Status::STATUS_BUSY);
		OpcUtils::MPSRegister reg;
		reg = registerOffset + OpcUtils::MPSRegister::ACTION_ID_IN;
		std::cout << "Setting register nr " + reg << std::endl;
		setNodeValue(client, registerNodes[reg], (uint16_t)command, reg);
		reg = registerOffset + OpcUtils::MPSRegister::DATA_IN;
		setNodeValue(client, registerNodes[reg], (uint16_t)payload1, reg);
		reg = registerOffset + OpcUtils::MPSRegister::DATA_IN;
		setNodeValue(client, registerNodes[reg], (uint16_t)payload2, reg);
		reg = registerOffset + OpcUtils::MPSRegister::STATUS_ENABLE_IN;
		setNodeValue(client, registerNodes[reg], statusBit, reg);
		reg = registerOffset + OpcUtils::MPSRegister::ERROR_IN;
		setNodeValue(client, registerNodes[reg], (uint8_t)error, reg);
	} catch (std::exception &e) {
		logger->warn("Error while sending command: {}", e.what());
		std::this_thread::sleep_for(opcua_poll_rate_);
		return false;
	}
	std::this_thread::sleep_for(opcua_poll_rate_);
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
	logger->warn("Trying to connect for machine");

	if (connection_mode_ == MOCKUP) {
		return;
	}
	while (!reconnect()) {
		std::this_thread::sleep_for(opcua_poll_rate_);
	}
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
		logger->flush_on(spdlog::level::info);
		logger->set_level(spdlog::level::info);
	}
	logger->info("\n\n\nNew logging session started");
}

bool
OpcUaMachine::reconnect()
{

	std::cout << "starting the reconnect!";
	disconnect();
	try {
		auto endpoint = OpcUtils::getEndpoint(ip_.c_str(), port_);
		auto config = UA_Client_getConfig(client);
		config->securityMode = UA_MESSAGESECURITYMODE_NONE;
//		config->certificateVerification = 
    	UA_ClientConfig_setDefault(config);
		logger->error("Connecting to: {}", endpoint);
		std::cout << "Connecting to: " <<  endpoint << std::endl;
		
		UA_StatusCode status = UA_Client_connect(client, endpoint.c_str());
		std::cout << "Hello from the connect?" << std::endl;
    	if(status != UA_STATUSCODE_GOOD) {
			std::cout << "UA_STATUSCODE != GOOD?!" << std::endl;
			UA_Client_delete(client);
			client = UA_Client_new();
			throw std::invalid_argument("UA_Statuscode is not good!");
			return false;
    	}
		// TODO Service_CreateSession();
		std::cout << "Was able to connect!" << std::endl;
		connected_ = true;
	} catch (const std::exception &exc) {
		logger->error("OPC UA connection error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
		UA_Client_delete(client);
		return false;
	} catch (...) {
		logger->error("Unknown error.");
		return false;
	}

	try {
		
		logger->error("Starting the subscription process!");
		//UA_NodeId basicNode = OpcUtils::getBasicNode(client, simulation_);

		//nodeIn    = OpcUtils::getInNode(client, simulation_);
		
		for (int i = 0; i < OpcUtils::MPSRegister::LAST; i++)
			registerNodes[i] = OpcUtils::getNode(client, (OpcUtils::MPSRegister)i, simulation_);
		
		subscribe(SUB_REGISTERS, simulation_);
		identify();
		update_callbacks();
		logger->error("Finished the subscriptions successfully!");
		return true;
	} catch (const std::exception &exc) {
		logger->error("Node path error: {} (@{}:{})", exc.what(), __FILE__, __LINE__);
		return false;
	} catch (...) {
		logger->error("Unknown error.");
		return false;
	}
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
	//subscriptions.clear();

	logger->info("Disconnecting");
	try {
		//client->Disconnect();
		logger->info("Disconnected");
		logger->flush();
		connected_ = false;
		//client.reset();
		return;
	} catch (std::exception &e) {
		logger->warn("Failed to disconnect: {}", e.what());
		try {
			logger->info("Aborting the connection");
			//client->Abort();
			logger->info("Aborted the connection");
			logger->flush();
			connected_ = false;
			//client.reset();
			return;
		} catch (std::exception &e) {
			logger->warn("Failed to abort: {}", e.what());
			try {
				logger->flush();
				connected_ = false;
				//client.reset();
				return;
			} catch (...) {
			}
		}
	}
	connected_ = false;
	//client.reset();
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

void
OpcUaMachine::subscribe(OpcUtils::MPSRegister reg, bool simulation)
{
	/*auto it = subscriptions.end();
	if ((it = subscriptions.find(reg)) != subscriptions.end())
		return it->second;
	//OpcUa::Node         node = OpcUtils::getNode(client.get(), reg, simulation);
	SubscriptionClient *sub  = new SubscriptionClient(logger);
	sub->reg                 = reg;
	//sub->node                = node;

	//int response_timeout = 100;
	//sub->subscription    = client->CreateSubscription(response_timeout, *sub);
	sub->handle          = sub->subscription->SubscribeDataChange(node);
	logger->info("Subscribed to {} (name: {}, handle: {})",
	             OpcUtils::REGISTER_NAMES[reg],
	             node.GetBrowseName().Name,
	             sub->handle);
	subscriptions.insert(SubscriptionClient::pair(reg, sub));
	return sub;*/
}

void
OpcUaMachine::cancelAllSubscriptions(bool log)
{
	if (log)
		printFinalSubscribtions();

	/*
	for (SubscriptionClient::map::iterator it = subscriptions.begin(); it != subscriptions.end();) {
		OpcUtils::MPSRegister reg = it->first;
		SubscriptionClient   *sub = it->second;
		sub->subscription->UnSubscribe(sub->handle);
		logger->info("Unsubscribed from {} (name: {}, handle: {})",
		             OpcUtils::REGISTER_NAMES[reg],
		             sub->node.GetBrowseName().Name,
		             sub->handle);
		it = subscriptions.erase(it);
	}
	*/
}

void
OpcUaMachine::cancelSubscription(OpcUtils::MPSRegister reg, bool log)
{
	/*auto it = subscriptions.find(reg);
	if (it != subscriptions.end()) {
		SubscriptionClient *sub = it->second;
		sub->subscription->UnSubscribe(sub->handle);
		logger->info("Unsubscribed from {} (name: {}, handle: {})",
		             OpcUtils::REGISTER_NAMES[reg],
		             sub->node.GetBrowseName().Name,
		             sub->handle);
		return subscriptions.erase(it);
	}
	return it;*/
}

OpcUtils::ReturnValue *
OpcUaMachine::getReturnValue(OpcUtils::MPSRegister reg)
{
	return nullptr;
}

bool
OpcUaMachine::setNodeValue(UA_Client* client, UA_NodeId node, boost::any val, OpcUtils::MPSRegister reg)
{
	/*SubscriptionClient::map::iterator it = subscriptions.find(reg);
	if (it != subscriptions.end())
		return OpcUtils::setNodeValue(node, val, it->second->mpsValue);
	return OpcUtils::setNodeValue(node, val);*/
	std::cout << "Trying to set node, connected = " << (connected_ ? "True" : "False") << std::endl;
	if(!connected_)
	{
		throw std::invalid_argument("Client not yet connected!");
	}
	std::cout << "In Set Node VAlue for " << node.identifierType << node.namespaceIndex << "!" << std::endl;
	UA_UInt16 value = boost::any_cast<uint16_t>(val);
	std::cout << "Casted" << std::endl;
	UA_Variant *newValue = UA_Variant_new();
	std::cout << "Created new variant" << std::endl;
	UA_Variant_setScalarCopy(newValue, &value, &UA_TYPES[UA_TYPES_UINT16]);
	std::cout << "Set variant value to " << boost::any_cast<uint16_t>(val) <<"!" << std::endl;
	UA_StatusCode ret =  UA_Client_writeValueAttribute(client, node, newValue);
	std::cout << "Writing value to server!" << std::endl;
	if(ret != UA_STATUSCODE_GOOD)
	{
		std::cout << "Writing of variable failed with value " << std::hex << ret << std::endl;
		throw std::invalid_argument("SetNodeValue - Something went wrong!"); 
	}
	return false;
}

void
OpcUaMachine::printFinalSubscribtions()
{
	logger->info("Final values of subscribed registers:");
}

/*void
OpcUaMachine::register_opc_callback( callback,
                                    OpcUtils::MPSRegister                   reg)
{
	logger->info("Registering callback for register {}", reg);
	SubscriptionClient *sub = subscribe(reg, simulation_);
	sub->add_callback(callback);
}*/

void
OpcUaMachine::update_callbacks()
{
	if (!connected_) {
		return;
	}
	/*for (const auto &cb : callbacks_) {
		register_opc_callback(cb.second, cb.first);
	}*/
}

void
OpcUaMachine::register_busy_callback(std::function<void(bool)> callback)
{
	/*if (callback) {
		callbacks_[OpcUtils::MPSRegister::STATUS_BUSY_IN] = [=](OpcUtils::ReturnValue *ret) {
			callback(ret->bool_s);
		};
	} else {
		callbacks_.erase(OpcUtils::MPSRegister::STATUS_BUSY_IN);
	}
	update_callbacks();*/
}

void
OpcUaMachine::register_ready_callback(std::function<void(bool)> callback)
{
	/*if (callback) {
		callbacks_[OpcUtils::MPSRegister::STATUS_READY_IN] = [=](OpcUtils::ReturnValue *ret) {
			callback(ret->bool_s);
		};
	} else {
		callbacks_.erase(OpcUtils::MPSRegister::STATUS_READY_IN);
	}
	update_callbacks();*/
}

void
OpcUaMachine::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	/*if (callback) {
		callbacks_[OpcUtils::MPSRegister::BARCODE_IN] = [=](OpcUtils::ReturnValue *ret) {
			callback(ret->bool_s);
		};
	} else {
		callbacks_.erase(OpcUtils::MPSRegister::BARCODE_IN);
	}
	update_callbacks();*/
}

void
OpcUaMachine::identify()
{
	send_instruction(
	  std::make_tuple(Command::COMMAND_SET_TYPE, get_type_from_station(machine_type_), 0, 0, 1, 0));
}

} // namespace mps_comm
} // namespace llsfrb
