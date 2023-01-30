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
: connected_(false),
  machine_type_(machine_type),
  ip_(ip),
  port_(port),
  connection_mode_(connection_mode),
  shutdown_(false),
  simulation_(connection_mode == SIMULATION)
{
	initLogger(log_path);
	callbacks_ = std::unordered_map<OpcUtils::MPSRegister, std::function<void(bool)>>();
	monitorMap = std::unordered_map<UA_UInt32, OpcUtils::MPSRegister>();
	subscribed_ = false;
	start_sending_instructions = false;
	running = true;
	worker_thread_ = std::thread(&OpcUaMachine::client_keep_alive, this);
	dispatcher_thread_ = std::thread(&OpcUaMachine::dispatch_command_queue, this);
}
void
OpcUaMachine::client_keep_alive()
{
	std::string prefix = "[client_keep_alive]";
	client_mutex_.lock();
	SetupClient();
	client_mutex_.unlock();
	auto url = "opc.tcp://" + ip_  + ":" + std::to_string(port_);
	for (int i = 0; i < OpcUtils::MPSRegister::LAST; i++)
		registerNodes[i] = OpcUtils::getNode(client, (OpcUtils::MPSRegister)i, simulation_);
    /* Endless loop runAsync */
    while(running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
		logger->info("{} Connecting to : " + url, prefix);
		logger->info("{} waiting on client_mutex!", prefix);
		client_mutex_.lock();
        UA_StatusCode retval = UA_Client_connect(client, url.c_str());
		switch(retval)
		{
			case UA_STATUSCODE_BADSERVERHALTED:
				/*logger->warn("Deleting client to retry!");
				UA_Client_delete(client);
				SetupClient();*/
				client_mutex_.unlock();
				UA_sleep_ms(1000);
				logger->info("{} releasing client_mutex!", prefix);
				continue;
			case UA_STATUSCODE_GOOD:
				break;
			default: 
				logger->warn( "{} Not connected. Retrying to connect in 1 second! Statuscode [{:x}]", prefix,retval);
				//UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Not connected. Retrying to connect in 1 second");
				/* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
				/* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
				client_mutex_.unlock();
				UA_sleep_ms(1000);
				logger->info("{} releasing client_mutex!", prefix);
				continue;
		}
        UA_Client_run_iterate(client, 300);
		client_mutex_.unlock();
		logger->info("{} releasing client_mutex!", prefix);
		UA_sleep_ms(40);
    };
    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
}

void
OpcUaMachine::dispatch_command_queue()
{
	std::string prefix = "[Dispatcher]";
	logger->info("{} Starting the dispatching thread!", prefix);
    while(running) {
		logger->info("{} waiting on command_queue_mutex_!", prefix);
		if(start_sending_instructions)
		{
			command_queue_mutex_.lock();
			if(!command_queue_.empty())
			{
				auto instruction = command_queue_.front();
				command_queue_mutex_.unlock();
				logger->info("{} releasing command_queue_mutex_!", prefix);
				if(send_instruction(instruction)){
					command_queue_.pop();
				}
				else
				{
					/*std::lock_guard<std::mutex> lg(client_mutex_);
					logger->warn("Wasn't able to send we recreate the client!");
					start_sending_instructions = false;	
					UA_Client_delete(client);
					SetupClient();*/
					UA_sleep_ms(1000);
				}
				// not able to send maybe new client
				UA_sleep_ms(40);
			}
			else{
				command_queue_mutex_.unlock();
				logger->info("{} releasing command_queue_mutex_!", prefix);
				logger->info("{} Nothing in the Command Queue!",prefix);
				UA_sleep_ms(200);
			}
		}
		else{

			logger->info("Not yet ready to send instructions!");
			UA_sleep_ms(1000);
		}
	}
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
	logger->info("Enqueued a instruction {} {} {} {} {} {}", command, payload1, payload2, timeout, status, error);
}

bool OpcUaMachine::send_instruction(const Instruction &instruction)

{
	const unsigned short command  = std::get<0>(instruction);
	const unsigned short payload1 = std::get<1>(instruction);
	const unsigned short payload2 = std::get<2>(instruction);
	const int            timeout  = std::get<3>(instruction);
	const unsigned char  status   = std::get<4>(instruction);
	const unsigned char  error    = std::get<5>(instruction);
	logger->info(
	  "Sending instruction {} {} {} {} {} {}", command, payload1, payload2, timeout, status, error);
	try {
		logger->info("[send_instruction] Waiting on the Client_mutex_!");
		client_mutex_.lock();
		OpcUtils::MPSRegister registerOffset;
		if (command < Station::STATION_BASE)
			registerOffset = OpcUtils::MPSRegister::ACTION_ID_BASIC;
		else
			registerOffset = OpcUtils::MPSRegister::ACTION_ID_IN;

		bool                  statusBit = (bool)(status & Status::STATUS_BUSY);
		OpcUtils::MPSRegister reg;

		reg = registerOffset + OpcUtils::MPSRegister::ACTION_ID_IN;
		if(!setNodeValue(client, registerNodes[reg], (uint16_t)command, reg))
			throw std::invalid_argument( "Couldn't set Node value" );
		//logger->info("Action ID got set to {}!",(uint16_t)command);
		reg = registerOffset + OpcUtils::MPSRegister::DATA0_IN;
		if(!setNodeValue(client, registerNodes[reg], (uint16_t)payload1, reg))
			throw std::invalid_argument( "Couldn't set Node value" );
		//logger->info("DATA_IN0 got set to {}!",(uint16_t)payload1);
		reg = registerOffset + OpcUtils::MPSRegister::DATA1_IN;
		if(!setNodeValue(client, registerNodes[reg], (uint16_t)payload2, reg))
			throw std::invalid_argument( "Couldn't set Node value" );
		//logger->info("DATA_IN1 got set to {}!",(uint16_t)payload2);
		reg = registerOffset + OpcUtils::MPSRegister::STATUS_ENABLE_IN;
		if(!setNodeValue(client, registerNodes[reg], statusBit, reg))
			throw std::invalid_argument( "Couldn't set Node value" );
		//logger->info("StatusEnable got set to {}!", statusBit ? "True": "False");
		reg = registerOffset + OpcUtils::MPSRegister::ERROR_IN;
		if(!setNodeValue(client, registerNodes[reg], (uint8_t)error, reg))
			throw std::invalid_argument( "Couldn't set Node value" );
		//logger->info("ErrorIn got set to {}!",(uint8_t)error);
	} catch (std::exception &e) {
		logger->warn("Error while sending command: {}", e.what());
		client_mutex_.unlock();
		logger->info("[send_instruction] released the client_mutex_!");
		std::this_thread::sleep_for(opcua_poll_rate_);
		return false;
	}
	client_mutex_.unlock();
	logger->info("[send_instruction] released the client_mutex_!");
	logger->info("send_instruction finished succesfull!");
	return true;
}

void OpcUaMachine::SetupClient()
{

	logger->info( "Setting up a new OPCUA Client!");
	client = UA_Client_new();
	auto config = UA_Client_getConfig(client);
	config->logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_FATAL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
	cc->clientContext = (void *) this;
	cc->timeout = 6000000;
    /* Set stateCallback */
    cc->stateCallback = mps_comm::stateCallback;
    cc->subscriptionInactivityCallback = mps_comm::subscriptionInactivityCallback;
}

void OpcUaMachine::reset()
{
	enqueue_instruction(Command::COMMAND_SET_TYPE, machine_type_ / 100);
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
	connected_ = false;
	start_sending_instructions = false;
	shutdown_ = true;
	running = false;
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
		logger->set_level(spdlog::level::info);
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
	UA_Variant *newValue = UA_Variant_new();
	//logger->info("Setting the value of {}",  node_name);
	if((reg >= OpcUtils::MPSRegister::STATUS_BUSY_IN && reg <= OpcUtils::MPSRegister::STATUS_ERROR_IN) ||  (reg >= OpcUtils::MPSRegister::STATUS_BUSY_BASIC && reg <= OpcUtils::MPSRegister::STATUS_ERROR_BASIC)) {
		//logger->info("Boolean value!");
		UA_Boolean bvalue = boost::any_cast<bool>(val);
		UA_Variant_setScalarCopy(newValue, &bvalue, &UA_TYPES[UA_TYPES_BOOLEAN]);
	}
	else if(reg == OpcUtils::MPSRegister::ERROR_BASIC || reg == OpcUtils::MPSRegister::ERROR_IN)
	{
		//logger->info("uint8_t value!");
		UA_Byte bytevalue = boost::any_cast<uint8_t>(val);
		UA_Variant_setScalarCopy(newValue, &bytevalue, &UA_TYPES[UA_TYPES_BYTE]);
	}
	else {
		//logger->info("Uint16 value!");
		UA_UInt16 ivalue = boost::any_cast<uint16_t>(val);
		UA_Variant_setScalarCopy(newValue, &ivalue, &UA_TYPES[UA_TYPES_UINT16]);
	}

	UA_StatusCode ret = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(4, cstring) , newValue);
	UA_Variant_delete(newValue);
	switch(ret)
	{
		case UA_STATUSCODE_GOOD:
			//logger->info("The setNode was successfull ({:x})",ret);
			return true;
		default:
			logger->warn("Writing of variable failed with value {:x}",ret);
			return false;
	}
	return false;
}



void
OpcUaMachine::register_busy_callback(std::function<void(bool)> callback)
{
	logger->info("Registering the busy callback!");
	if (callback) {
		OpcUaMachine::callbacks_[OpcUtils::MPSRegister::STATUS_BUSY_IN] = callback;
	} else {
		OpcUaMachine::callbacks_.erase(OpcUtils::MPSRegister::STATUS_BUSY_IN);
	}
}

void
OpcUaMachine::register_ready_callback(std::function<void(bool)> callback)
{
	logger->info("Registering the Ready callback!");
	if (callback) {
		OpcUaMachine::callbacks_[OpcUtils::MPSRegister::STATUS_READY_IN] = callback;
	} else {
		OpcUaMachine::callbacks_.erase(OpcUtils::MPSRegister::STATUS_READY_IN);
	}
}

void
OpcUaMachine::register_barcode_callback(std::function<void(unsigned long)> callback)
{
	logger->info("Registering the barcode callback!");
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

void stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
	auto machine = (OpcUaMachine *) UA_Client_getContext(client);
	std::string prefix = "[stateCallback]";
	machine->logger->info("recoveryStatus = {:x}", recoveryStatus);
	switch(recoveryStatus)
	{
		case UA_STATUSCODE_BADTIMEOUT:
		case UA_STATUSCODE_BADSERVERHALTED:
			machine->logger->info("{} Recreating the client as recoveryStatus is {:x} which is fatal",prefix, recoveryStatus);
			machine->logger->info("{} waiting on client_mutex!", prefix);
			machine->client_mutex_.lock();
			UA_Client_delete(client);
			machine->SetupClient();
			machine->client_mutex_.unlock();
			machine->logger->info("{} releasing client_mutex!", prefix);
			return;
			break;
		case UA_STATUSCODE_GOOD:

			break;
		default: 
			machine->logger->info("{} Skipping the stateCallback with recoveryStatus {:x}", prefix, recoveryStatus);
			return;
	}
    switch(channelState) {
    case UA_SECURECHANNELSTATE_FRESH:
	    machine->logger->info("{} The client is not yet connected", prefix);
		break;
    case UA_SECURECHANNELSTATE_CLOSED:
        machine->logger->info("{} The client is disconnected", prefix);
		machine->start_sending_instructions = false;
        break;
    case UA_SECURECHANNELSTATE_HEL_SENT:
        machine->logger->info("{} Waiting for ack", prefix);
        break;
    case UA_SECURECHANNELSTATE_OPN_SENT:
        machine->logger->info("{} Waiting for OPN Response", prefix);
        break;
    case UA_SECURECHANNELSTATE_OPEN:
        machine->logger->info("{} A SecureChannel to the server is open", prefix);
        break;
    default:
        break;
    }

    switch(sessionState) {
    case UA_SESSIONSTATE_ACTIVATED: {
        machine->logger->info("{} A session with the server is activated", prefix);
        /* A new session was created. We need to create the subscription. */
        /* Create a subscription */
        	UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
			request.requestedLifetimeCount = -1;
			//request.requestedPublishingInterval = 300;
			request.requestedMaxKeepAliveCount = -1;
        	UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request, NULL, NULL, deleteSubscriptionCallback);
            if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
			{
               machine->logger->info("{} Create subscription {} succeeded!", prefix, response.subscriptionId);
			   machine->subscriptionId = response.subscriptionId;
			}
            else
			{
				machine->logger->info("{} Couldn't create subscription?", prefix);
                return;
			}
            /* Add a MonitoredItem */
			if(!registerMonitoredItem(client, machine->subscriptionId, OpcUtils::MPSRegister::STATUS_BUSY_IN))
			{
				return;
			}
			if(!registerMonitoredItem(client, machine->subscriptionId, OpcUtils::MPSRegister::STATUS_READY_IN))
			{
				return;
			}
			//TODO activate once we fix everything up for real machines: 
			/*if(!registerMonitoredItem(client, machine->subscriptionId, OpcUtils::MPSRegister::BARCODE_IN))
			{
				return;
			}*/
			if((StationType)(machine->machine_type_ / 100) == StationType::STATION_TYPE_RS)
			{
				if(!registerMonitoredItem(client, machine->subscriptionId, OpcUtils::MPSRegister::SLIDECOUNT_IN))
				{
					return;
				}
			}
			machine->logger->info("{} All subscriptions done, starting with sending tasks!", prefix);
			//TODO activate once we fix everything up for real machines: machine->reset();
			machine->start_sending_instructions = true;

	        }
        break;

    case UA_SESSIONSTATE_CREATE_REQUESTED:
			machine->logger->info("UA_SESSIONSTATE_CREATE_REQUESTED!");
		break;
    case UA_SESSIONSTATE_CREATED:
		machine->logger->info("UA_SESSIONSTATE_CREATED!");
		break;
    case UA_SESSIONSTATE_ACTIVATE_REQUESTED:
		machine->logger->info("UA_SESSIONSTATE_ACTIVATE_REQUESTED!");
		break;
    case UA_SESSIONSTATE_CLOSING:
		machine->logger->info("UA_SESSIONSTATE_CLOSING!");
		break;
    case UA_SESSIONSTATE_CLOSED:
        machine->logger->info("Session disconnected");
        break;
    default:
        break;
    }
}

void stopHandler(int sign) {
	
}

void ValueChangeCallback(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    auto machine = (OpcUaMachine *) UA_Client_getContext(client);
    machine->logger->info("ValueChangeCallback has been called!");
	auto iter = machine->callbacks_.find(machine->monitorMap[monId]);
	if( iter == machine->callbacks_.end())
	{
		machine->logger->info("No Callback for this monitored object is registered!");
		return;
	}
	if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN])){
		UA_Boolean raw_data = *(UA_Boolean *) value->value.data;
		machine->logger->info("Callback = {} for id {}", raw_data, monId);
		machine->callbacks_[machine->monitorMap[monId]]((bool)raw_data);
		return;
	}
	if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_UINT16]))
	{
		UA_UInt16 raw_data = *(UA_UInt16 *) value->value.data;
		machine->logger->info("Callback = {} for id {}", raw_data, monId);
		machine->callbacks_[machine->monitorMap[monId]]((short)raw_data);
		return;
	}
	if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_UINT32]))
	{
		UA_UInt32 raw_data = *(UA_UInt32 *) value->value.data;
		machine->logger->info("Callback = {} for id {}", raw_data, monId);
		machine->callbacks_[machine->monitorMap[monId]]((uint)raw_data);
		return;
	}

	machine->logger->error("Callback for unsupported type?");
}

void deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    auto machine = (OpcUaMachine *) UA_Client_getContext(client);
    machine->logger->info("Subscription Id %u was deleted", subscriptionId);
}

void subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
	auto machine = (OpcUaMachine *) UA_Client_getContext(client);
    machine->logger->info("Inactivity for subscription {} trying to renew!", subId);
	return;
	auto reg = machine->monitorMap[subId];
	auto result = registerMonitoredItem(client, machine->subscriptionId, reg);
	
	if(result)
	{
		machine->logger->info("Succesfull resubscribe!");
		return;
	}
	machine->logger->info("Couldn't resubscribe!");
}

void deleteMonitoredItemCallback(UA_Client *client, unsigned int subId, void *subContext, unsigned int monId, void *monContext)
{
	auto machine = (OpcUaMachine *) UA_Client_getContext(client);
	machine->logger->info("Delete for Monitored Item [{}]-[{}] is called!", subId, monId);
}

bool registerMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,  OpcUtils::MPSRegister reg)
{
	auto nodeName = OpcUtils::getNode(client, reg, false);
	auto machine = (OpcUaMachine *) UA_Client_getContext(client);
	char cstring[1024];
	strcpy(cstring, nodeName.c_str());
	UA_NodeId subscribeNode = UA_NODEID_STRING(4, cstring);
	UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(subscribeNode);
	
	UA_MonitoredItemCreateResult monResponse =  UA_Client_MonitoredItems_createDataChange(client, subscriptionId, UA_TIMESTAMPSTORETURN_SOURCE, monRequest, NULL, ValueChangeCallback, deleteMonitoredItemCallback);
	if(monResponse.statusCode == UA_STATUSCODE_GOOD)
	{
		machine->logger->info("Subscribed to {} with id {}", nodeName.c_str(), monResponse.monitoredItemId);
		machine->monitorMap[monResponse.monitoredItemId] = reg;
		return true;
	}
	machine->logger->info("Monitoring failed with error code {:x}",monResponse.statusCode);
	return false;		
}

} // namespace mps_comm
} // namespace llsfrb
