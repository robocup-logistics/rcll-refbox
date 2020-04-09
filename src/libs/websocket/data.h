/***************************************************************************
 *  data.h - class to provide shared data object to interact with clients
 *
 *  Created: Mon Mar 30 22:59:00 2020
 *  Copyright  2020       Daniel Swoboda  [swoboda@kbsg.rwth-aachen.de]
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

#ifndef _PLUGINS_WEBSOCKET_DATA_H_
#define _PLUGINS_WEBSOCKET_DATA_H_

#include "client.h"
#include "logging/logger.h"

#include <rapidjson/document.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace llsfrb::websocket {

class Data
{
public:
	Data(Logger *logger_);
	std::string log_pop();
	void        log_push(std::string log);
	void        log_push(rapidjson::Document &d);
	bool        log_empty();
	void        log_wait();
	void        clients_add(std::shared_ptr<Client> client);
	void        clients_send_all(std::string msg);
	void        clients_send_all(rapidjson::Document &d);

private:
	Logger *                             logger_;
	std::mutex                           log_mu;
	std::mutex                           cli_mu;
	std::condition_variable              log_cv;
	std::queue<std::string>              logs;
	std::vector<std::shared_ptr<Client>> clients;
};

} // namespace llsfrb::websocket

#endif