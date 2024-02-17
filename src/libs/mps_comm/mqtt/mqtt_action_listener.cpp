/***************************************************************************
*  mqtt_action_listener.cpp -MQTT action listener
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

#include "mqtt_action_listener.h"

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

void
mqtt_action_listener::on_failure(const mqtt::token &tok)
{
	logger_->info("Failure for token: [{}]", std::to_string(tok.get_message_id()));
}

void
mqtt_action_listener::on_success(const mqtt::token &tok)
{
	auto top = tok.get_topics();
	if (top && !top->empty()) {
		return;
	}
	logger_->info("Success for token: [{}] with topic {}",
	              std::to_string(tok.get_message_id()),
	              (*top)[0]);
}

mqtt_action_listener::mqtt_action_listener(const std::string              &name,
                                           std::shared_ptr<spdlog::logger> logger)
: name_(name), logger_(logger)
{
}

mqtt_action_listener::~mqtt_action_listener()
{
}

} // namespace mps_comm
} // namespace llsfrb
