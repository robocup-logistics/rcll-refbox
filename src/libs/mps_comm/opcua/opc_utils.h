/***************************************************************************
 *  opc_utils.h - Helper functions for OPC-UA
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

#pragma once
#include <open62541.h>
#if HAVE_SYSTEM_SPDLOG
#	include <spdlog/sinks/basic_file_sink.h>
#	include <spdlog/sinks/stdout_sinks.h>
#endif
#include <boost/any.hpp>
#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

// when printing recursively, indent is used to make the hirarchy more visible
#define logIndent(i)                \
	{                                 \
		i * 2, (i + 1) * 2, (i + 2) * 2 \
	}

class OpcUtils
{
public:
	// Return value container for a subscription to a OPC UA Node
	struct ReturnValue
	{
	private:
		bool               isEmpty_  = true;
		bool               isVector_ = false;

	public:
		uint16_t    uint16_s;
		uint32_t    uint32_s;
		uint64_t    uint64_s;
		int16_t     int16_s;
		int32_t     int32_s;
		int64_t     int64_s;
		float_t     float_s;
		double_t    double_s;
		bool        bool_s;
		uint8_t     uint8_s;
		std::string string_s;

		std::vector<uint16_t>    uint16_v;
		std::vector<uint32_t>    uint32_v;
		std::vector<uint64_t>    uint64_v;
		std::vector<int16_t>     int16_v;
		std::vector<int32_t>     int32_v;
		std::vector<int64_t>     int64_v;
		std::vector<float_t>     float_v;
		std::vector<double_t>    double_v;
		std::vector<bool>        bool_v;
		std::vector<uint8_t>     uint8_v;
		std::vector<std::string> string_v;

	};

	// Registers existing in the MPS, to which it is possible to subscribe to
	enum MPSRegister {
		ACTION_ID_IN = 0,
		BARCODE_IN,
		DATA_IN,
		ERROR_IN,
		SLIDECOUNT_IN,
		STATUS_BUSY_IN,
		STATUS_ENABLE_IN,
		STATUS_ERROR_IN,
		STATUS_READY_IN,
		ACTION_ID_BASIC,
		BARCODE_BASIC,
		DATA_BASIC,
		ERROR_BASIC,
		SLIDECOUNT_BASIC,
		STATUS_BUSY_BASIC,
		STATUS_ENABLE_BASIC,
		STATUS_ERROR_BASIC,
		STATUS_READY_BASIC,
		LAST, // must be the last in the list
	};
	// MPS Register addition operator overload
	friend MPSRegister operator+(const MPSRegister a, const MPSRegister b);

	// static vector containing all the names corresponding to the MPSRegisters; usage example: REGISTER_NAMES[MPSRegister::ACTION_ID_IN]
	static const std::vector<std::string> REGISTER_NAMES;
	// static vector containing the path from root node to the "basic" node in the MPS
	static const std::vector<std::string> BASIC_NODE_PATH;
	// static vector containing the path from root node to the "in" node in the MPS
	static const std::vector<std::string> IN_NODE_PATH;
	// static vector containing the path from root node to the "basic" node in the MPS simulation
	static const std::vector<std::string> BASIC_NODE_PATH_SIM;
	// static vector containing the path from root node to the "in" node in the MPS simulation
	static const std::vector<std::string> IN_NODE_PATH_SIM;
	// static vector containing the path from root node to the "basic" node in the 64 bit MPS simulation
	static const std::vector<std::string> BASIC_NODE_PATH_64_SIM;
	// static vector containing the path from root node to the "in" node in the 64 bit MPS simulation
	static const std::vector<std::string> IN_NODE_PATH_64_SIM;


	// Set OPC UA node value; if retVal is set, the SubscriptionClient internal return value is overridden
	static bool
	setNodeValue(UA_NodeId node, boost::any val, OpcUtils::ReturnValue *retVal = nullptr);

	// Get OPC UA Endpoint given by IP and port
	static std::string getEndpoint(const char *ip, unsigned short port);
	// Get OPC UA node using MPSRegister
	static UA_NodeId getNode(UA_Client *client, MPSRegister reg, bool simulation = false);
	// Get OPC UA Node value as OPC UA Variant with the needed type
	static void getNodeValueWithCorrectType(UA_NodeId node, boost::any val);
	// Get "basic" OPC UA node
	static UA_NodeId getBasicNode(UA_Client *client, bool simulation = false);
	// Get "in" OPC UA node
	static UA_NodeId getInNode(UA_Client *client, bool simulation = false);
	// Get OPC UA node relative path (path after in or basic node) by MPSRegister
	static const std::string getNodeRelativePath(MPSRegister reg);
	// Get OPC UA node full path by MPSRegister
	static const std::vector<std::string> getNodeFullPath(MPSRegister reg);
	// Get OPC UA node full path by MPSRegister for a MPS simulation
	static const std::vector<std::string> getNodeFullPathSim(MPSRegister reg);
	// Get OPC UA node full path by MPSRegister for a 64 bit MPS simulation
	static const std::vector<std::string> getNodeFullPath64Sim(MPSRegister reg);
};
} // namespace mps_comm
} // namespace llsfrb
