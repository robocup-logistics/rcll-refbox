// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  mqtt_utils.cpp - MQTT_LEGACY utility definitions
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

#include "mqtt_utils.h"

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

Topic
MqttLegacyUtils::ParseTopic(std::string topic)
{
	bool in       = false;
	bool basic    = false;
	bool ready    = false;
	bool busy     = false;
	bool slidecnt = false;
	if (topic.find("/In/") != std::string::npos) {
		in = true;
	}
	if (topic.find("/Basic/") != std::string::npos) {
		basic = true;
	}
	if (topic.find("/" + MqttLegacyUtils::bits[0]) != std::string::npos) {
		busy = true;
	}
	if (topic.find("/" + MqttLegacyUtils::bits[1]) != std::string::npos) {
		ready = true;
	}
	if (topic.find("/" + MqttLegacyUtils::registers[5]) != std::string::npos) {
		slidecnt = true;
	}

	if (in && ready)
		return Topic::InNodes_Ready;
	if (in && busy)
		return Topic::InNodes_Busy;
	if (basic && ready)
		return Topic::InNodes_Ready;
	if (basic && busy)
		return Topic::InNodes_Busy;
	if (in && slidecnt)
		return Topic::InNodes_Sldcnt;
	return Topic::Ignored;
}

std::string
MqttLegacyUtils::BuildTopic(std::string name, std::string target_register, bool in)
{
	std::stringstream s;
	s << TOPIC_PREFIX << "/" << name << "/" << (in ? folders[0] : folders[1]) << "/"
	  << target_register;
	return s.str();
}

} // namespace mps_comm
} // namespace rcll
