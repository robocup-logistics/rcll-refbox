/***************************************************************************
 *  mqtt_utils.h - MQTT utility definitions
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

#pragma once
#include <mqtt/async_client.h>

#include <sstream>

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
enum Topic {
	Ignored          = 0,
	InNodes_Ready    = 1,
	InNodes_Busy     = 2,
	InNodes_Sldcnt   = 3,
	BasicNodes_Ready = 4,
	BasicNodes_Busy  = 5,
};

class MqttUtils
{
public:
	const inline static std::string              BROKER_ADDRESS = "tcp://mosquitto:1883";
	const inline static std::string              TOPIC_PREFIX   = "MPS";
	const inline static std::string              TOPIC          = "MPS/{0}/{1}/{2}";
	const inline static std::vector<std::string> folders        = {"In", "Basic"};
	const inline static std::vector<std::string> registers =
	  {"ActionId", "BarCode", "Data/Data[0]", "Data/Data[1]", "Error", "SlideCnt", "Status"};
	const inline static std::vector<std::string> bits =
	  {"Busy", "Ready", "Error", "Enable", "unused0", "unused1", "inSensor", "outSensor"};
	const inline static int                       N_RETRY_ATTEMPTS = 5;
	const inline static int                       QOS              = 2;
	const inline static std::chrono::milliseconds mqtt_delay_{20};

	static Topic       ParseTopic(std::string topic);
	static std::string BuildTopic(std::string name, std::string target_register, bool in);
};

} // namespace mps_comm
} // namespace llsfrb