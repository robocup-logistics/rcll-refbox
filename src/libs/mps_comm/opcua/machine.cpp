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
		logger->info( "Creating new OPCUA Client!");
		auto config = UA_Client_getConfig(client);
		config->logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_FATAL);
	}

	subscribed_ = false;
	worker_thread_ = std::thread(&OpcUaMachine::dispatch_command_queue, this);
}

std::shared_ptr<spdlog::logger> OpcUaMachine::logger = std::shared_ptr<spdlog::logger>(nullptr);


void
OpcUaMachine::dispatch_command_queue()
{

    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    /* Set stateCallback */
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
	running = true;
	for (int i = 0; i < OpcUtils::MPSRegister::LAST; i++)
		registerNodes[i] = OpcUtils::getNode(client, (OpcUtils::MPSRegister)i, simulation_);
    /* Endless loop runAsync */
    while(running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
			logger->info( "Not connected. Retrying to connect in 1 second!");

            //UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Not connected. Retrying to connect in 1 second");
            /* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
            /* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
            UA_sleep_ms(1000);
            continue;
        }
        UA_Client_run_iterate(client, 1000);
		if(start_sending_instructions)
		{
			std::unique_lock<std::mutex> lock(command_queue_mutex_);
			if(!command_queue_.empty())
			{
				auto instruction = command_queue_.front();
				lock.unlock();
				send_instruction(instruction);
				command_queue_.pop();
			}
			else{
				logger->info( "No Commands in the queue!");
			}
		}
    };

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
}

std::unordered_map<OpcUtils::MPSRegister, std::function<void(bool)>> OpcUaMachine::callbacks_ = std::unordered_map<OpcUtils::MPSRegister, std::function<void(bool)>>();
std::unordered_map<UA_UInt32,OpcUtils::MPSRegister > OpcUaMachine::monitorMap = std::unordered_map<UA_UInt32, OpcUtils::MPSRegister>();
bool OpcUaMachine::start_sending_instructions = false;

void OpcUaMachine::stopHandler(int sign) {
	logger->info("Received Ctrl-C");
}

void OpcUaMachine::ValueChangeCallback(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    logger->info("ValueChangeCallback has been called!");
	if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN])){
		UA_Boolean raw_data = *(UA_Boolean *) value->value.data;
		logger->info("Callback = %d for id %d", raw_data, monId);
		OpcUaMachine::callbacks_[OpcUaMachine::monitorMap[monId]]((bool)raw_data);
	}
}

void OpcUaMachine::deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    logger->info("Subscription Id %u was deleted", subscriptionId);
}

void OpcUaMachine::subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    logger->info("Inactivity for subscription %u", subId);
}

void OpcUaMachine::stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    switch(channelState) {
    case UA_SECURECHANNELSTATE_FRESH:
    case UA_SECURECHANNELSTATE_CLOSED:
        logger->info("The client is disconnected");
        break;
    case UA_SECURECHANNELSTATE_HEL_SENT:
        logger->info( "Waiting for ack");
        break;
    case UA_SECURECHANNELSTATE_OPN_SENT:
        logger->info("Waiting for OPN Response");
        break;
    case UA_SECURECHANNELSTATE_OPEN:
        logger->info("A SecureChannel to the server is open");
        break;
    default:
        break;
    }

    switch(sessionState) {
    case UA_SESSIONSTATE_ACTIVATED: {
        logger->info("A session with the server is activated");
        /* A new session was created. We need to create the subscription. */
        /* Create a subscription */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        UA_CreateSubscriptionResponse response =
            UA_Client_Subscriptions_create(client, request, NULL, NULL, deleteSubscriptionCallback);
            if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
			{
               logger->info("Create subscription succeeded, id %u",response.subscriptionId);
			}
            else
			{
				logger->info("Couldn't create subscription?");
                return;
			}
            /* Add a MonitoredItem */
			registerMonitoredItem(client, response.subscriptionId, OpcUtils::MPSRegister::STATUS_BUSY_IN);
			registerMonitoredItem(client, response.subscriptionId, OpcUtils::MPSRegister::STATUS_READY_IN);
        }
        break;
    case UA_SESSIONSTATE_CLOSED:
        logger->info("Session disconnected");
        break;
    default:
        break;
    }
}

bool OpcUaMachine::registerMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,  OpcUtils::MPSRegister reg)
{
	auto nodeName = OpcUtils::getNode(client, OpcUtils::MPSRegister::STATUS_BUSY_IN, false);
	char cstring[1024];
	strcpy(cstring, nodeName.c_str());
	UA_NodeId subscribeNode = UA_NODEID_STRING(4, cstring);
	UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(subscribeNode);
	UA_MonitoredItemCreateResult monResponse =  UA_Client_MonitoredItems_createDataChange(client, subscriptionId, UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL, ValueChangeCallback, NULL);
	if(monResponse.statusCode == UA_STATUSCODE_GOOD)
	{
		logger->info("Subscribed to %s with id %u", nodeName.c_str(), monResponse.monitoredItemId);
		start_sending_instructions = true;
		monitorMap[monResponse.monitoredItemId] = reg;
		return true;
	}
	logger->info("Monitoring failed with error code %x",monResponse.statusCode);
	return false;		
}

void OpcUaMachine::enqueue_instruction(unsigned short command,
                                  unsigned short payload1,
                                  unsigned short payload2,
                                  int            timeout,
                                  unsigned char  status,
                                  unsigned char  error)
{
	std::lock_guard<std::mutex> lg(command_queue_mutex_);
	command_queue_.push(std::make_tuple(command, payload1, payload2, timeout, status, error));
}

bool OpcUaMachine::send_instruction(const Instruction &instruction)

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
	return true;
}

void OpcUaMachine::reset()
{
	enqueue_instruction(machine_type_ | Command::COMMAND_RESET);
}

void OpcUaMachine::connect()
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

void OpcUaMachine::set_light(llsf_msgs::LightColor color,
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

void OpcUaMachine::conveyor_move(ConveyorDirection direction, MPSSensor sensor)
{
	enqueue_instruction(Command::COMMAND_MOVE_CONVEYOR + machine_type_,
	                    sensor,
	                    direction,
	                    Timeout::TIMEOUT_BAND);
}

void OpcUaMachine::reset_light()
{
	set_light(llsf_msgs::LightColor::RED, llsf_msgs::OFF);
}

void OpcUaMachine::initLogger(const std::string &log_path)
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

bool OpcUaMachine::reconnect()
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
		
		identify();
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

bool
OpcUaMachine::setNodeValue(UA_Client* client, std::string node_name, boost::any val, OpcUtils::MPSRegister reg)
{
	char cstring[1024];
	strcpy(cstring, node_name.c_str());
	UA_UInt16 value = boost::any_cast<uint16_t>(val);
	UA_Variant *newValue = UA_Variant_new();
	UA_Variant_setScalarCopy(newValue, &value, &UA_TYPES[UA_TYPES_UINT16]);
	UA_StatusCode ret = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(4, cstring) , newValue);
	UA_Variant_delete(newValue);
	if(ret == UA_STATUSCODE_GOOD)
	{
		std::cout << "The setNode was successfull" << std::endl;
	}
	else
	{
		std::cout << "Writing of variable failed with value " << std::hex << ret << std::endl;
	}
	return false;
}



void
OpcUaMachine::register_busy_callback(std::function<void(bool)> callback)
{
	if (callback) {
		OpcUaMachine::callbacks_[OpcUtils::MPSRegister::STATUS_BUSY_IN] = callback;
	} else {
		OpcUaMachine::callbacks_.erase(OpcUtils::MPSRegister::STATUS_BUSY_IN);
	}
}

void
OpcUaMachine::register_ready_callback(std::function<void(bool)> callback)
{
	if (callback) {
		OpcUaMachine::callbacks_[OpcUtils::MPSRegister::STATUS_READY_IN] = callback;
	} else {
		OpcUaMachine::callbacks_.erase(OpcUtils::MPSRegister::STATUS_READY_IN);
	}
}

void
OpcUaMachine::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	if (callback) {
		OpcUaMachine::callbacks_[OpcUtils::MPSRegister::BARCODE_IN] = callback;
	} else {
		OpcUaMachine::callbacks_.erase(OpcUtils::MPSRegister::BARCODE_IN);
	}
}

void
OpcUaMachine::identify()
{
	send_instruction(
	  std::make_tuple(Command::COMMAND_SET_TYPE, get_type_from_station(machine_type_), 0, 0, 1, 0));
}

} // namespace mps_comm
} // namespace llsfrb
