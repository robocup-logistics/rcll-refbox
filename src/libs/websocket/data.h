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
#include <rapidjson/schema.h>

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
	Data(std::shared_ptr<Logger> logger, CLIPS::Environment *env, fawkes::Mutex &env_mutex);
	std::string                                      log_pop();
	void                                             log_push(std::string log);
	void                                             log_push(rapidjson::Document &d);
	bool                                             log_empty();
	void                                             log_wait();
	void                                             clients_add(std::shared_ptr<Client> client);
	void                                             clients_send_all(std::string msg);
	void                                             clients_send_all(rapidjson::Document &d);
	void                                             log_push_attention_message(std::string text,
	                                                                            std::string team,
	                                                                            int         time_to_display,
	                                                                            float       game_time);
	std::function<void(std::string)>                 clips_set_gamestate;
	std::function<void(std::string)>                 clips_set_gamephase;
	std::function<void(std::string, std::string)>    clips_set_confval;
	std::function<void()>                            clips_randomize_field;
	std::function<void(std::string, std::string)>    clips_set_teamname;
	std::function<void(int, bool, int, std::string)> clips_confirm_delivery;
	std::function<void(std::string, int)>            clips_set_order_delivered;
	std::function<void(std::string)>                 clips_production_machine_add_base;
	std::function<void(std::string, std::string)>    clips_production_set_machine_state;
	std::function<void(int, std::string, bool)>      clips_robot_set_robot_maintenance;
	std::function<void(std::string, std::string)>    clips_production_reset_machine_by_team;
	std::function<void(int, std::string, float, std::string, std::string)> clips_add_points_team;
	bool        match(CLIPS::Fact::pointer &fact, std::string tmpl_name);
	void        log_push_points();
	void        log_push_config(std::string path);
	void        log_push_ring_spec();
	void        log_push_game_state();
	void        log_push_time_info();
	void        log_push_robot_info(int number, std::string name);
	void        log_push_agent_task_info(int tid, int rid);
	void        log_push_order_info(int id);
	void        log_push_machine_info(std::string name);
	void        log_push_workpiece_info(int id);
	void        log_push_order_info_via_delivery(int delivery_id);
	void        log_push_known_teams();
	std::string on_connect_known_teams();
	std::string on_connect_agent_task_info();
	std::string on_connect_machine_info();
	std::string on_connect_order_info();
	std::string on_connect_workpiece_info();
	std::string on_connect_robot_info();
	std::string on_connect_game_state();
	std::string on_connect_ring_spec();
	std::string on_connect_points();
	std::string on_connect_config();
	std::string get_gamestate();
	std::string get_gamephase();
	std::map<std::string, std::shared_ptr<rapidjson::SchemaDocument>> command_schema_map;
	template <class T>
	void
	get_known_teams_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void
	get_machine_info_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void
	get_order_info_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void
	get_robot_info_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void get_agent_task_info_fact(T                                  *o,
	                              rapidjson::Document::AllocatorType &alloc,
	                              CLIPS::Fact::pointer                fact);
	template <class T>
	void
	get_game_state_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void
	get_time_info_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void
	get_ring_spec_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void get_points_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void get_config_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact);
	template <class T>
	void get_workpiece_info_fact(T                                  *o,
	                             rapidjson::Document::AllocatorType &alloc,
	                             CLIPS::Fact::pointer                fact);

	rapidjson::Value get_unconfirmed_delivery_fact(rapidjson::Document::AllocatorType &alloc,
	                                               int64_t                             id);
	std::string      on_connect_info(std::string tmpl_name,
	                                 void (Data::*get_info_fact)(rapidjson::Value *,
                                                          rapidjson::Document::AllocatorType &,
                                                          CLIPS::Fact::pointer));

	rapidjson::Document
	pack_facts_to_doc(std::string tmpl_name,
	                  void (Data::*get_info_fact)(rapidjson::Value *,
	                                              rapidjson::Document::AllocatorType &,
	                                              CLIPS::Fact::pointer));
	rapidjson::Document
	pack_facts_to_doc(std::string                              tmpl_name,
	                  const std::vector<CLIPS::Fact::pointer> &facts,
	                  void (Data::*get_info_fact)(rapidjson::Value *,
	                                              rapidjson::Document::AllocatorType &,
	                                              CLIPS::Fact::pointer));

private:
	std::shared_ptr<Logger>                    logger_;
	std::mutex                                 log_mu;
	std::mutex                                 cli_mu;
	std::condition_variable                    log_cv;
	std::queue<std::string>                    logs;
	std::vector<std::shared_ptr<Client>>       clients;
	std::shared_ptr<CLIPS::Environment>        env_;
	fawkes::Mutex                             &env_mutex_;
	std::shared_ptr<rapidjson::SchemaDocument> load_schema(std::string path);
};

} // namespace llsfrb::websocket

#endif
