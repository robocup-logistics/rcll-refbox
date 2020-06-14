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

#include <clipsmm.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

using namespace fawkes;
namespace llsfrb::websocket {
class Client; // forward declaration

class Data
{
public:
	Data(Logger *logger, CLIPS::Environment *env_, fawkes::Mutex &env_mutex_);
	std::string log_pop();
	void        log_push(std::string log);
	void        log_push(rapidjson::Document &d);
	bool        log_empty();
	void        log_wait();
	void        clients_add(std::shared_ptr<Client> client);
	void        clients_send_all(std::string msg);
	void        clients_send_all(rapidjson::Document &d);
	void        log_push_fact_gamestate(std::string time,
	                                    std::string state,
	                                    std::string phase,
	                                    std::string prevphase,
	                                    std::string team_cyan,
	                                    std::string team_magenta);
	void        log_push_fact_gamepoints(std::string points_cyan, std::string points_magenta);
	void        log_push_attention_message(std::string text, std::string team, std::string time);

	std::function<void(std::string)>                 clips_set_gamestate;
	std::function<void(std::string)>                 clips_set_gamephase;
	std::function<void()>                            clips_randomize_field;
	std::function<void(std::string, std::string)>    clips_set_teamname;
	std::function<void(int, bool, int, std::string)> clips_confirm_delivery;
	std::function<void(std::string, int)>            clips_set_order_delivered;
	std::function<void(std::string)>                 clips_production_machine_add_base;
	std::function<void(std::string, std::string)>    clips_production_set_machine_state;
	std::function<void(int, std::string, bool)>      clips_robot_set_robot_maintenance;
	bool match(CLIPS::Fact::pointer &fact, std::string tmpl_name);

private:
	Logger *                             logger_;
	std::mutex                           log_mu;
	std::mutex                           cli_mu;
	std::condition_variable              log_cv;
	std::queue<std::string>              logs;
	std::vector<std::shared_ptr<Client>> clients;
	CLIPS::Environment *                 env_;
	fawkes::Mutex &                      env_mutex_;
};

} // namespace llsfrb::websocket

#endif