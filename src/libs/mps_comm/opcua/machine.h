/***************************************************************************
 *  machine.h - OPC-UA communication with an MPS
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

// Abstract base class for the stations
#pragma once

#include "../machine.h"
#include "mps_io_mapping.h"
#include "opc_utils.h"
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <open62541.h>
#include <boost/any.hpp>
#include <spdlog/spdlog.h>

namespace llsfrb {
namespace mps_comm {

enum ConveyorDirection { FORWARD = 1, BACKWARD = 2 };

enum MPSSensor { INPUT = 1, MIDDLE = 2, OUTPUT = 3 };

class MachineFactory;

class OpcUaMachine : public virtual Machine
{
	using Instruction =
	  std::tuple<unsigned short, unsigned short, unsigned short, int, unsigned char, unsigned char>;

	friend class MachineFactory;

public:
	OpcUaMachine(Station            machine_type,
	             const std::string &ip,
	             unsigned short     port,
	             const std::string &log_path = "",
	             ConnectionMode              = PLC);

	~OpcUaMachine() override;

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
	void register_busy_callback(std::function<void(bool)>) override;
	void register_ready_callback(std::function<void(bool)>) override;
	void register_barcode_callback(std::function<void(unsigned long)>) override;
	// Identify: The PLC does not know, which machine it runs. This command tells it the type.
	virtual void identify();

protected:
	void connect();
	void enqueue_instruction(unsigned short command,
	                         unsigned short payload1 = 0,
	                         unsigned short payload2 = 0,
	                         int            timeout  = 0,
	                         unsigned char  status   = 1,
	                         unsigned char  error    = 0);
	bool send_instruction(const Instruction &instruction);
	void dispatch_command_queue();
	void update_callbacks();

	// OPC UA related methods
	// Connect to OPC UA Server using IP and PORT
	bool reconnect();
	// Disconnect from OPC UA Server
	void disconnect();
	// Initialize logger; If log_path is empty, the logs are redirected to
	// std::cout, else they are saved to the in log_path specified file
	void initLogger(const std::string &log_path);
	// Helper function to set OPC UA Node value correctly
	bool setNodeValue(UA_Client* client, UA_NodeId node, boost::any val, OpcUtils::MPSRegister reg);
	// Helper function to get ReturnValue correctly
	OpcUtils::ReturnValue *getReturnValue(OpcUtils::MPSRegister reg);


	// Subscribe to multiple specified MPSRegisters; If ReturnValues are set, the
	// SubscriptionClients internal ReturnValues are overridden
	void subscribe(std::vector<OpcUtils::MPSRegister> registers, bool simulation = false);
	void subscribe(OpcUtils::MPSRegister reg, bool simulation);
	// Subscribe to all existing MPSRegisters
	void subscribeAll(bool simulation = false);
	// Cancel a subscription given a specific MPSRegister; If default argument is
	// true, final subscription value will be printed before deleting
	void cancelSubscription(OpcUtils::MPSRegister reg, bool log = false);
	// Cancel all existing subscriptions; If default argument is true, final
	// subscription values will be printed before deleting
	void cancelAllSubscriptions(bool log = false);
	// Print the final subscription values
	void printFinalSubscribtions();

	const Station     machine_type_;
	const std::string ip_;
	unsigned short    port_;

	const ConnectionMode connection_mode_;

	bool                    shutdown_;
	std::mutex              command_queue_mutex_;
	std::mutex              command_mutex_;
	std::condition_variable queue_condition_;
	std::queue<Instruction> command_queue_;
	std::thread             worker_thread_;

	bool connected_;
	bool simulation_;

	//std::unordered_map<OpcUtils::MPSRegister, SubscriptionClient::ReturnValueCallback> callbacks_;

	// OPC UA related variables

	/* OVERRIDE */
	// OVERRIDE: static vector containing the MPSRegisters to automatically subscribe; This should be overwritten if needed
	static const std::vector<OpcUtils::MPSRegister> SUB_REGISTERS;

	// OPC UA logger
	std::shared_ptr<spdlog::logger> logger;
	// OPC UA Client pointer
	UA_Client *client;
	// OPC UA Nodes for each subscribable MPSRegister
	UA_NodeId registerNodes[OpcUtils::MPSRegister::LAST];
	// OPC UA Input Register for station Jobs
	UA_NodeId nodeIn;
	// OPC UA Input Register for Basic Jobs
	UA_NodeId nodeBasic;
	// All subscriptions to MPSRegisters in form map<MPSRegister, Subscription>
	//SubscriptionClient::map subscriptions;
};

} // namespace mps_comm
} // namespace llsfrb
