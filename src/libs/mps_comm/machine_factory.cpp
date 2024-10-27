// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  machine_factory.cpp - Create MPS machine instances
 *
 *  Created: Tue 21 Jan 2020 12:53:24 CET 12:53
 *  Copyright  2020  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "machine_factory.h"

#include "mockup/stations.h"

#ifdef HAVE_FREEOPCUA
#	include "opcua/stations.h"
#endif

#ifdef HAVE_MQTT
#	include "mqtt/stations.h"
#	include "mqtt_legacy/stations.h"
#endif

#include <core/exception.h>

namespace rcll {
namespace mps_comm {
MachineFactory::MachineFactory(std::shared_ptr<Configuration> config) : config_(config) {};

std::unique_ptr<Machine>
MachineFactory::create_machine(const std::string &name,
                               const std::string &type,
                               const std::string &ip,
                               unsigned int       port,
                               const std::string &log_path,
                               const std::string &connection_mode)
{
#ifdef HAVE_FREEOPCUA
	if (connection_mode == "plc" || connection_mode == "plc_simulation") {
		OpcUaMachine::ConnectionMode mode;
		if (connection_mode == "plc") {
			mode = OpcUaMachine::PLC;
		} else if (connection_mode == "plc_simulation") {
			mode = OpcUaMachine::SIMULATION;
		} else {
			throw fawkes::Exception("Unexpected connection mode '%s' for machine '%s'",
			                        connection_mode.c_str(),
			                        name.c_str());
		}
		std::unique_ptr<OpcUaMachine> mps;
		if (type == "BS") {
			mps = std::make_unique<OpcUaBaseStation>(name, ip, port, log_path, mode);
		} else if (type == "CS") {
			mps = std::make_unique<OpcUaCapStation>(name, ip, port, log_path, mode);
		} else if (type == "RS") {
			mps = std::make_unique<OpcUaRingStation>(name, ip, port, log_path, mode);
		} else if (type == "DS") {
			mps = std::make_unique<OpcUaDeliveryStation>(name, ip, port, log_path, mode);
		} else if (type == "SS") {
			mps = std::make_unique<OpcUaStorageStation>(name, ip, port, log_path, mode);
		} else {
			throw fawkes::Exception("Unexpected machine type '%s' for machine '%s'",
			                        type.c_str(),
			                        name.c_str());
		}
		// Do not connect just now; instead, let it connect in the background.
		//mps->connect();
		return std::move(mps);
	}
#endif
#ifdef HAVE_MOCKUP
	if (connection_mode == "mockup") {
		float exec_speed = config_->get_float_or_default("llsfrb/simulation/speedup", 1);
		if (type == "BS") {
			return std::make_unique<MockupBaseStation>(name, exec_speed);
		} else if (type == "CS") {
			return std::make_unique<MockupCapStation>(name, exec_speed);
		} else if (type == "DS") {
			return std::make_unique<MockupDeliveryStation>(name, exec_speed);
		} else if (type == "RS") {
			return std::make_unique<MockupRingStation>(name, exec_speed);
		} else if (type == "SS") {
			return std::make_unique<MockupStorageStation>(name, exec_speed);
		} else {
			throw fawkes::Exception(
			  "Unexpected machine type '%s' for machine '%s' and connection mode '%s'",
			  type.c_str(),
			  name.c_str(),
			  connection_mode.c_str());
		}
	}
#endif
#ifdef HAVE_MQTT
	if (connection_mode == "mqtt") {
		std::unique_ptr<MqttMachine> mps;
		if (type == "BS") {
			mps = std::make_unique<MqttBaseStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
			return std::move(mps);
		} else if (type == "CS") {
			return std::make_unique<MqttCapStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else if (type == "DS") {
			return std::make_unique<MqttDeliveryStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else if (type == "RS") {
			return std::make_unique<MqttRingStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else if (type == "SS") {
			return std::make_unique<MqttStorageStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else {
			throw fawkes::Exception(
			  "Unexpected machine type '%s' for machine '%s' and connection mode '%s'",
			  type.c_str(),
			  name.c_str(),
			  connection_mode.c_str());
		}
	}
	if (connection_mode == "mqtt_legacy") {
		std::unique_ptr<MqttLegacyMachine> mps;
		if (type == "BS") {
			mps = std::make_unique<MqttLegacyBaseStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
			return std::move(mps);
		} else if (type == "CS") {
			return std::make_unique<MqttLegacyCapStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else if (type == "DS") {
			return std::make_unique<MqttLegacyDeliveryStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else if (type == "RS") {
			return std::make_unique<MqttLegacyRingStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else if (type == "SS") {
			return std::make_unique<MqttLegacyStorageStation>(
			  name, ip, port, log_path, MqttMachine::ConnectionMode::MQTT);
		} else {
			throw fawkes::Exception(
			  "Unexpected machine type '%s' for machine '%s' and connection mode '%s'",
			  type.c_str(),
			  name.c_str(),
			  connection_mode.c_str());
		}
	}
#endif
	throw fawkes::Exception("Unexpected connection mode '%s' for machine '%s'",
	                        connection_mode.c_str(),
	                        name.c_str());
}
} // namespace mps_comm
} // namespace rcll
