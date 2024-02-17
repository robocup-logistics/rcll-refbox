/***************************************************************************
*  mqtt_action_listener.h -MQTT action listener
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
#include <spdlog/logger.h>

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

class mqtt_action_listener : public virtual mqtt::iaction_listener
{
	std::string                     name_;
	std::shared_ptr<spdlog::logger> logger_;
	void                            on_failure(const mqtt::token &tok) override;
	void                            on_success(const mqtt::token &tok) override;

public:
	mqtt_action_listener(const std::string &name, std::shared_ptr<spdlog::logger> logger);
	~mqtt_action_listener();
};

} // namespace mps_comm
} // namespace llsfrb
