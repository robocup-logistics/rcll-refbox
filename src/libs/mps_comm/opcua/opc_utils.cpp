/***************************************************************************
 *  opc_utils.cpp - Helper functions for OPC-UA
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

#include "opc_utils.h"

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

// MPS Register related stuff

OpcUtils::MPSRegister
operator+(const OpcUtils::MPSRegister a, const OpcUtils::MPSRegister b)
{
	return (OpcUtils::MPSRegister)((int)a + (int)b);
}

// OpcUtils related stuff

// TODO change from this paths
const std::string in_path = "|var|CPX-E-CEC-C1-PN.Application.G.In";
const std::string basic_path = "|var|CPX-E-CEC-C1-PN.Application.G.Basic";
//const std::string in_path = "DeviceSet/CPX-E-CEC-C1-PN/Resources/Application/GlobalVars/G/In";
//const std::string basic_path = "DeviceSet/CPX-E-CEC-C1-PN/Resources/Application/GlobalVars/G/Basic";
const std::string in_sim_path = "DeviceSet/CODESYS Control Win V3 x64/Resources/Application/GlobalVars/G/In"; 
const std::string basic_sim_path = "DeviceSet/CODESYS Control Win V3 x64/Resources/Application/GlobalVars/G/Basic";
// to 
/*
const std::string in_path = "|var|CPX-E-CEC-C1-PN.Application.G.In";
const std::string basic_path = "|var|CPX-E-CEC-C1-PN.Application.G.Basic";
*/
// Static values

const std::vector<std::string> OpcUtils::REGISTER_NAMES({"ACTION_ID_IN",
                                                         "BARCODE_IN",
                                                         "DATA_IN",
                                                         "ERROR_IN",
                                                         "SLIDECOUNT_IN",
                                                         "STATUS_BUSY_IN",
                                                         "STATUS_ENABLE_IN",
                                                         "STATUS_ERROR_IN",
                                                         "STATUS_READY_IN",
                                                         "ACTION_ID_BASIC",
                                                         "BARCODE_BASIC",
                                                         "DATA_BASIC",
                                                         "ERROR_BASIC",
                                                         "SLIDECOUNT_BASIC",
                                                         "STATUS_BUSY_BASIC",
                                                         "STATUS_ENABLE_BASIC",
                                                         "STATUS_ERROR_BASIC",
                                                         "STATUS_READY_BASIC"});

/* CHANGE THE PATHS ONCE REAL MPS IS USED */
const std::vector<std::string> OpcUtils::BASIC_NODE_PATH({"Objects",
                                                          "2:DeviceSet",
                                                          "4:CPX-E-CEC-C1-PN",
                                                          "4:Resources",
                                                          "4:Application",
                                                          "3:GlobalVars",
                                                          "4:G",
                                                          "4:Basic"});
const std::vector<std::string> OpcUtils::IN_NODE_PATH({"Objects",
                                                       "2:DeviceSet",
                                                       "4:CPX-E-CEC-C1-PN",
                                                       "4:Resources",
                                                       "4:Application",
                                                       "3:GlobalVars",
                                                       "4:G",
                                                       "4:In"});
const std::vector<std::string> OpcUtils::BASIC_NODE_PATH_SIM({"Objects",
                                                              "2:DeviceSet",
                                                              "4:CODESYS Control Win V3",
                                                              "3:Resources",
                                                              "4:Application",
                                                              "3:GlobalVars",
                                                              "4:G",
                                                              "4:Basic"});
const std::vector<std::string> OpcUtils::IN_NODE_PATH_SIM({"Objects",
                                                           "2:DeviceSet",
                                                           "4:CODESYS Control Win V3",
                                                           "3:Resources",
                                                           "4:Application",
                                                           "3:GlobalVars",
                                                           "4:G",
                                                           "4:In"});
const std::vector<std::string> OpcUtils::BASIC_NODE_PATH_64_SIM({"Objects",
                                                                 "2:DeviceSet",
                                                                 "4:CODESYS Control Win V3 x64",
                                                                 "3:Resources",
                                                                 "4:Application",
                                                                 "3:GlobalVars",
                                                                 "4:G",
                                                                 "4:Basic"});
const std::vector<std::string> OpcUtils::IN_NODE_PATH_64_SIM({"Objects",
                                                              "2:DeviceSet",
                                                              "4:CODESYS Control Win V3 x64",
                                                              "3:Resources",
                                                              "4:Application",
                                                              "3:GlobalVars",
                                                              "4:G",
                                                              "4:In"});



// Set functions

bool
OpcUtils::setNodeValue(UA_NodeId node, boost::any val, OpcUtils::ReturnValue *retVal)
{
	/*
	OpcUa::Variant var = OpcUtils::getNodeValueWithCorrectType(node, val);
	if (retVal != nullptr)
		retVal->setValue(var);
	node.SetValue(var); */
	return true;
}

// Get functions

std::string
OpcUtils::getEndpoint(const char *ip, unsigned short port)
{
	std::string endpoint;
	endpoint =
	  std::string("opc.tcp://") + ip + std::string(":") + std::to_string(port) + std::string("/");
	/* // if connection requires user and password:
  endpoint.EndpointUrl = std::string("opc.tcp://") + this->user + ":" + this->password + std::string("@") + this->ip + ":" + std::to_string(this->port) + std::string("/");*/
	return endpoint;
}

std::string
OpcUtils::getNode(UA_Client *client, MPSRegister reg, bool simulation)
{
	char cstring[1024];
	if (!simulation)
	{
		std::string relativePath = getNodeRelativePath(reg);
		std::string fullpath;
		if(reg >= MPSRegister::ACTION_ID_BASIC)
		{
			fullpath = basic_path + relativePath;
		}
		else
		{
			fullpath = in_path + relativePath;
		}
		strcpy(cstring, fullpath.c_str());
		return fullpath;
	}
	strcpy(cstring, basic_sim_path.c_str());
	//return UA_NODEID_STRING(4, cstring);
	return basic_sim_path;

}

void
OpcUtils::getNodeValueWithCorrectType(UA_NodeId node, boost::any val)
{
	/*
	switch (node.GetValue().Type()) {
	case OpcUa::VariantType::UINT16: return static_cast<uint16_t>(boost::any_cast<uint16_t>(val));
	case OpcUa::VariantType::UINT32: return static_cast<uint32_t>(boost::any_cast<uint32_t>(val));
	case OpcUa::VariantType::UINT64: return static_cast<uint64_t>(boost::any_cast<uint64_t>(val));
	case OpcUa::VariantType::INT16: return static_cast<int16_t>(boost::any_cast<int16_t>(val));
	case OpcUa::VariantType::INT32: return static_cast<int32_t>(boost::any_cast<int32_t>(val));
	case OpcUa::VariantType::INT64: return static_cast<int64_t>(boost::any_cast<int64_t>(val));
	case OpcUa::VariantType::FLOAT: return static_cast<float_t>(boost::any_cast<float_t>(val));
	case OpcUa::VariantType::DOUBLE: return static_cast<double_t>(boost::any_cast<double_t>(val));
	case OpcUa::VariantType::BOOLEAN: return static_cast<bool>(boost::any_cast<bool>(val));
	case OpcUa::VariantType::BYTE: return static_cast<uint8_t>(boost::any_cast<uint8_t>(val));
	default: return static_cast<int>(boost::any_cast<int>(val));
	}*/
}

UA_NodeId
OpcUtils::getBasicNode(UA_Client *client, bool simulation)
{
	char cstring[1024];
	if (!simulation)
	{

		strcpy(cstring, basic_path.c_str());
		return UA_NODEID_STRING(4, cstring);
	}
	strcpy(cstring, basic_sim_path.c_str());
	return UA_NODEID_STRING(4, cstring);
}

UA_NodeId
OpcUtils::getInNode(UA_Client *client, bool simulation)
{
	char cstring[1024];
	if (!simulation)
	{
		strcpy(cstring, in_path.c_str());
		return UA_NODEID_STRING(4, cstring);
	}
	strcpy(cstring, in_sim_path.c_str());
	return UA_NODEID_STRING(4, cstring);
}

const std::string
OpcUtils::getNodeRelativePath(MPSRegister reg)
{

	// TODO change from /p/xxx => .p.xxx
	switch (reg) {
	case MPSRegister::ACTION_ID_IN:
	case MPSRegister::ACTION_ID_BASIC: return ".p.ActionId";
	case MPSRegister::BARCODE_IN:
	case MPSRegister::BARCODE_BASIC: return ".p.BarCode";
	case MPSRegister::DATA_IN:
	case MPSRegister::DATA_BASIC: return ".p.Data";
	case MPSRegister::DATA0_IN:
	case MPSRegister::DATA0_BASIC: return ".p.Data[0]";
	case MPSRegister::DATA1_IN:
	case MPSRegister::DATA1_BASIC: return ".p.Data[1]";
	case MPSRegister::ERROR_IN:
	case MPSRegister::ERROR_BASIC: return ".p.Error";
	case MPSRegister::SLIDECOUNT_IN:
	case MPSRegister::SLIDECOUNT_BASIC: return ".p.SlideCnt";
	case MPSRegister::STATUS_BUSY_IN:
	case MPSRegister::STATUS_BUSY_BASIC: return ".p.Status.Busy";
	case MPSRegister::STATUS_ENABLE_IN:
	case MPSRegister::STATUS_ENABLE_BASIC: return ".p.Status.Enable";
	case MPSRegister::STATUS_ERROR_IN:
	case MPSRegister::STATUS_ERROR_BASIC: return ".p.Status.Error";
	case MPSRegister::STATUS_READY_IN:
	case MPSRegister::STATUS_READY_BASIC: return ".p.Status.Ready";
	default: return "";
	}
}

const std::vector<std::string>
OpcUtils::getNodeFullPath(MPSRegister reg)
{
	std::vector<std::string> ret;
	if (reg >= MPSRegister::ACTION_ID_BASIC)
		ret = std::vector<std::string>(BASIC_NODE_PATH);
	else
		ret = std::vector<std::string>(IN_NODE_PATH);

	/*for (std::string p : getNodeRelativePath(reg))
		ret.push_back(p);*/
	return ret;
}

const std::vector<std::string>
OpcUtils::getNodeFullPathSim(MPSRegister reg)
{
	std::vector<std::string> ret;
	if (reg >= MPSRegister::ACTION_ID_BASIC)
		ret = std::vector<std::string>(BASIC_NODE_PATH_SIM);
	else
		ret = std::vector<std::string>(IN_NODE_PATH_SIM);

	/*for (std::string p : getNodeRelativePath(reg))
		ret.push_back(p);*/
	return ret;
}

const std::vector<std::string>
OpcUtils::getNodeFullPath64Sim(MPSRegister reg)
{
	std::vector<std::string> ret;
	if (reg >= MPSRegister::ACTION_ID_BASIC)
		ret = std::vector<std::string>(BASIC_NODE_PATH_64_SIM);
	else
		ret = std::vector<std::string>(IN_NODE_PATH_64_SIM);

	/*for (std::string p : getNodeRelativePath(reg))
		ret.push_back(p);*/
	return ret;
}
} // namespace mps_comm
} // namespace llsfrb
