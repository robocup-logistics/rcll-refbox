/***************************************************************************
 *  data.cpp - class to provide shared data object to interact with clients
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

#include "data.h"

#include <core/threading/mutex_locker.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace llsfrb::websocket {

/**
 * @brief Construct a new Data:: Data object
 * 
 * @param logger_ logger to be used
 */
Data::Data(Logger *logger_, CLIPS::Environment *env_, fawkes::Mutex &env_mutex_)
: logger_(logger_), env_(env_), env_mutex_(env_mutex_)
{
	logger_->log_info("Websocket", "loading JSON schemas for command validation");

	std::string base_path      = std::getenv("LLSF_REFBOX_DIR");
	std::string schema_names[] = {"confirm_delivery",
	                              "machine_add_base",
	                              "randomize_field",
	                              "set_gamephase",
	                              "set_gamestate",
	                              "set_machine_state",
	                              "set_order_delivered",
	                              "set_robot_maintenance",
	                              "set_teamname"};

	for (const std::string &schema_name : schema_names) {
		rapidjson::SchemaDocument *sd =
		  load_schema(base_path + "/src/libs/websocket/message_schemas/" + schema_name + ".json");
		if (sd) {
			command_schema_map[schema_name] = sd;
		} else {
			throw Exception("No schema file could be found for '%s'", schema_name.c_str());
		}
	}
}

/**
 * @brief Read a JSON schema file and return pointer to a rapidjson SchemaDocument
 * 
 * @param path 
 * @return rapidjson::SchemaDocument 
 */
rapidjson::SchemaDocument *
Data::load_schema(std::string path)
{
	rapidjson::Document d;

	FILE *fp = std::fopen(path.c_str(), "rb");
	if (fp == 0) {
		logger_->log_error("Websocket", "couldn't load JSON schema file");
		return nullptr;
	}
	char                      readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	if (d.ParseStream(is).HasParseError()) {
		logger_->log_error("Websocket", "couldn't parse JSON schema file");
		return nullptr;
	}
	rapidjson::SchemaDocument *schema = new rapidjson::SchemaDocument(d);
	return schema;
}

/**
 * @brief get first element from log queue
 * 
 *  This thread-safe function returns the first element from the log message queue and removes it.
 * 
 * @return std::string first element from log queue
 */
std::string
Data::log_pop()
{
	const std::lock_guard<std::mutex> lock(log_mu);
	std::string                       log = logs.front();
	logs.pop();
	return log;
}

/**
 * @brief add element to log queue
 * 
 * This thread-safe function adds an element to the log message queue. 
 * 
 * @param log element (std::string) to be added
 */
void
Data::log_push(std::string log)
{
	const std::lock_guard<std::mutex> lock(log_mu);
	logs.push(log);
	log_cv.notify_one();
}

/**
 * @brief add JSON element to log queue
 * 
 * This thread-safe function adds a JSON element to the log message queue. 
 * 
 * @param d element (rapidjson::Document) to be added
 */
void
Data::log_push(rapidjson::Document &d)
{
	const std::lock_guard<std::mutex>          lock(log_mu);
	rapidjson::StringBuffer                    buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);

	logs.push(buffer.GetString());
	log_cv.notify_one();
}

/**
 * @brief check if log queue is empty
 * 
 * @return true log queue is empty
 * @return false log queue is not empty
 */
bool
Data::log_empty()
{
	return logs.empty();
}

/**
 * @brief blocks calling thread until log queue is not-empty
 * 
 */
void
Data::log_wait()
{
	std::unique_lock<std::mutex> lock(log_mu);
	while (log_empty()) //loop to avoid spurious wake-ups
	{
		log_cv.wait(lock);
	}
}

/**
 * @brief add client for handling
 * 
 *  Add a new connected client to the vector of currently active clients. 
 *  The client will be handled, e.g. for log distribution. 
 * 
 * @param client client to be added
 */
void
Data::clients_add(std::shared_ptr<Client> client)
{
	const std::lock_guard<std::mutex> lock(cli_mu);
	clients.push_back(client);
}

/**
 * @brief send one message to all clients
 * 
 *  Sends the given message to all connected clients. 
 *  Handles disconnects and removes the respective clients.
 * 
 * @param msg message to be sent
 */
void
Data::clients_send_all(std::string msg)
{
	const std::lock_guard<std::mutex> lock(cli_mu);

	std::vector<std::shared_ptr<Client>> unfailed_clients;

	for (auto const &client : clients) {
		if (client->active) {
			if (client->send(msg)) {
				unfailed_clients.push_back(client);
			} else {
				client->disconnect();
			}
		}
	}
	clients = unfailed_clients;
}

/**
 * @brief send one JSON document to all clients
 * 
 *  Converts given JSON document to string and calls clients_send_all(std::string msg). 
 * 
 * @param d JSON document to be sent
 */
void
Data::clients_send_all(rapidjson::Document &d)
{
	rapidjson::StringBuffer                    buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	clients_send_all(buffer.GetString());
}

/**
 * @brief log attention message
 * 
 * Called from the CLIPS environment when an attention message is generated, forward that to the clients
 * 
 * @param text 
 * @param team 
 * @param time 
 */
void
Data::log_push_attention_message(std::string text, std::string team, std::string time)
{
	rapidjson::Document d;
	d.SetObject();
	rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

	rapidjson::Value json_string;
	json_string.SetString("attention", alloc);
	d.AddMember("level", json_string, alloc);
	json_string.SetString((text).c_str(), alloc);
	d.AddMember("text", json_string, alloc);
	json_string.SetString((team).c_str(), alloc);
	d.AddMember("team", json_string, alloc);
	json_string.SetString((time).c_str(), alloc);
	d.AddMember("time", json_string, alloc);

	log_push(d);
}

/** Get a value from a fact.
 * @param fact pointer to CLIPS fact
 * @param slot_name name of field to retrieve
 * @return template-specific return value
 */
template <typename T>
T
get_value(const CLIPS::Fact::pointer &fact, const std::string &slot_name)
{
	CLIPS::Values v = fact->slot_value(slot_name);
	if (v.empty()) {
		throw Exception("No value for slot '%s'", slot_name.c_str());
	}
	if (v[0].type() == CLIPS::TYPE_SYMBOL && v[0].as_string() == "nil") {
		return T();
	}
	return v[0];
}

/** Specialization for bool.
 * @param fact pointer to CLIPS fact
 * @param slot_name name of field to retrieve
 * @return boolean value
 */
template <>
bool
get_value(const CLIPS::Fact::pointer &fact, const std::string &slot_name)
{
	CLIPS::Values v = fact->slot_value(slot_name);
	if (v.empty()) {
		throw Exception("No value for slot '%s'", slot_name.c_str());
	}
	if (v[0].type() != CLIPS::TYPE_SYMBOL) {
		throw Exception("Value for slot '%s' is not a boolean", slot_name.c_str());
	}
	return (v[0].as_string() == "TRUE");
}

/** Get value array.
 * This is not a template because the overly verbose operator API
 * of CLIPS::Value can lead to ambiguous overloads, e.g., resolving
 * std::string to std::string or const char * operators.
 * @param fact pointer to CLIPS fact
 * @param slot_name name of field to retrieve
 * @return vector of strings from multislot
 */
static std::vector<std::string>
get_values(const CLIPS::Fact::pointer &fact, const std::string &slot_name)
{
	CLIPS::Values            v = fact->slot_value(slot_name);
	std::vector<std::string> rv(v.size());
	for (size_t i = 0; i < v.size(); ++i) {
		switch (v[i].type()) {
		case CLIPS::TYPE_FLOAT: rv[i] = std::to_string(static_cast<double>(v[i])); break;
		case CLIPS::TYPE_INTEGER: rv[i] = std::to_string(static_cast<long long int>(v[i])); break;
		case CLIPS::TYPE_SYMBOL:
		case CLIPS::TYPE_STRING:
		case CLIPS::TYPE_INSTANCE_NAME: rv[i] = static_cast<std::string &>(v[i]); break;
		default: rv[i] = "CANNOT-REPRESENT"; break;
		}
	}
	return rv;
}

/**
 * @brief Check if the given fact pointer is an instance of the given template
 * 
 * @param fact 
 * @param tmpl_name 
 * @return true 
 * @return false 
 */
bool
Data::match(CLIPS::Fact::pointer &fact, std::string tmpl_name)
{
	CLIPS::Template::pointer tmpl = fact->get_template();
	if (tmpl->name() != tmpl_name)
		return false;
	return true;
}

/**
 * @brief Gets all machine-info facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_machine_info()
{
	MutexLocker          lock(&env_mutex_);
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "machine")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("machine-info", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "name")).c_str(), alloc);
				d.AddMember("name", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
				d.AddMember("team", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "mtype")).c_str(), alloc);
				d.AddMember("mtype", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
				d.AddMember("state", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "zone")).c_str(), alloc);
				d.AddMember("zone", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "bs-side")).c_str(), alloc);
				d.AddMember("bs_side", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "bs-color")).c_str(), alloc);
				d.AddMember("bs_color", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "rs-ring-color")).c_str(), alloc);
				d.AddMember("rs_ring_color", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "cs-operation")).c_str(), alloc);
				d.AddMember("cs_operation", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "rotation")));
				d.AddMember("rotation", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "bases-added")));
				d.AddMember("bases_added", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "bases-used")));
				d.AddMember("bases_used", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "ds-order")));
				d.AddMember("ds_order", json_string, alloc);
				json_string.SetBool((get_value<bool>(fact, "cs-retrieved")));
				d.AddMember("cs_retrieved", json_string, alloc);

				rapidjson::Value lights_array(rapidjson::kArrayType);
				lights_array.Reserve(get_values(fact, "actual-lights").size(), alloc);
				for (const auto &e : get_values(fact, "actual-lights")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					lights_array.PushBack(v, alloc);
				}
				d.AddMember("actual_lights", lights_array, alloc);

				rapidjson::Value ring_array(rapidjson::kArrayType);
				ring_array.Reserve(get_values(fact, "rs-ring-colors").size(), alloc);
				for (const auto &e : get_values(fact, "rs-ring-colors")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					ring_array.PushBack(v, alloc);
				}
				d.AddMember("rs_ring_colors", ring_array, alloc);

				//send it off bye bye
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type machine");
			}
		}
		fact = fact->next();
	}
}

/**
 * @brief Gets all order-info facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_order_info()
{
	MutexLocker lock(&env_mutex_);

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "order")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("order-info", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "complexity")).c_str(), alloc);
				d.AddMember("complexity", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "base-color")).c_str(), alloc);
				d.AddMember("base_color", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "cap-color")).c_str(), alloc);
				d.AddMember("cap_color", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "id")));
				d.AddMember("id", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "quantity-requested")));
				d.AddMember("quantity_requested", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "delivery-gate")));
				d.AddMember("delivery_gate", json_string, alloc);
				json_string.SetBool((get_value<bool>(fact, "competitive")));
				d.AddMember("competitive", json_string, alloc);
				json_string.SetBool((get_value<bool>(fact, "active")));
				d.AddMember("active", json_string, alloc);

				rapidjson::Value delivery_array(rapidjson::kArrayType);
				delivery_array.Reserve(get_values(fact, "delivery-period").size(), alloc);
				for (const auto &e : get_values(fact, "delivery-period")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					delivery_array.PushBack(v, alloc);
				}
				d.AddMember("delivery_period", delivery_array, alloc);

				rapidjson::Value quantity_array(rapidjson::kArrayType);
				quantity_array.Reserve(get_values(fact, "quantity-delivered").size(), alloc);
				for (const auto &e : get_values(fact, "quantity-delivered")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					quantity_array.PushBack(v, alloc);
				}
				d.AddMember("quantity_delivered", quantity_array, alloc);

				rapidjson::Value ring_array(rapidjson::kArrayType);
				ring_array.Reserve(get_values(fact, "ring-colors").size(), alloc);
				for (const auto &e : get_values(fact, "ring-colors")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					ring_array.PushBack(v, alloc);
				}
				d.AddMember("ring_colors", ring_array, alloc);

				//send it off bye bye
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type order");
			}
		}
		fact = fact->next();
	}
}

/**
 * @brief Gets all robot-info facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_robot_info()
{
	MutexLocker lock(&env_mutex_);

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "robot")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("robot-info", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
				d.AddMember("state", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
				d.AddMember("team", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "team-color")).c_str(), alloc);
				d.AddMember("team_color", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "name")).c_str(), alloc);
				d.AddMember("name", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "host")).c_str(), alloc);
				d.AddMember("host", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "number")));
				d.AddMember("number", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "port")));
				d.AddMember("port", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "maintenance-start-time")));
				d.AddMember("maintenance_start-time", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "maintenance-cycles")));
				d.AddMember("maintenance_cylces", json_string, alloc);
				json_string.SetBool((get_value<bool>(fact, "has-pose")));
				d.AddMember("has_pose", json_string, alloc);
				json_string.SetBool((get_value<bool>(fact, "maintenance-warning-sent")));
				d.AddMember("maintenance_warning_sent", json_string, alloc);

				rapidjson::Value last_seen_array(rapidjson::kArrayType);
				last_seen_array.Reserve(get_values(fact, "last-seen").size(), alloc);
				for (const auto &e : get_values(fact, "last-seen")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					last_seen_array.PushBack(v, alloc);
				}
				d.AddMember("last_seen", last_seen_array, alloc);

				rapidjson::Value pose_array(rapidjson::kArrayType);
				pose_array.Reserve(get_values(fact, "pose").size(), alloc);
				for (const auto &e : get_values(fact, "pose")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					pose_array.PushBack(v, alloc);
				}
				d.AddMember("pose", pose_array, alloc);

				//send it off
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type robot");
			}
		}
		fact = fact->next();
	}
}

/**
 * @brief Gets all game-state facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_game_state()
{
	MutexLocker lock(&env_mutex_);

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "gamestate")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("gamestate", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
				d.AddMember("state", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "phase")).c_str(), alloc);
				d.AddMember("phase", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "prev-phase")).c_str(), alloc);
				d.AddMember("prev_phase", json_string, alloc);
				json_string.SetFloat((get_value<float>(fact, "game-time")));
				d.AddMember("game_time", json_string, alloc);
				json_string.SetBool((get_value<bool>(fact, "over-time")));
				d.AddMember("over_time", json_string, alloc);
				json_string.SetString((get_values(fact, "teams")[0]).c_str(), alloc);
				d.AddMember("cyan", json_string, alloc);
				json_string.SetString((get_values(fact, "teams")[1]).c_str(), alloc);
				d.AddMember("magenta", json_string, alloc);
				json_string.SetString((get_values(fact, "points")[0]).c_str(), alloc);
				d.AddMember("points_cyan", json_string, alloc);
				json_string.SetString((get_values(fact, "points")[1]).c_str(), alloc);
				d.AddMember("points_magenta", json_string, alloc);

				//send it off
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type gamestate");
			}
		}
		fact = fact->next();
	}
}

/**
 * @brief Gets all ring-spec facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_ring_spec()
{
	MutexLocker lock(&env_mutex_);

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "ring-spec")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("ring-spec", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "color")).c_str(), alloc);
				d.AddMember("color", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "req-bases")));
				d.AddMember("req_bases", json_string, alloc);

				//send it off
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type ring-spec");
			}
		}
		fact = fact->next();
	}
}

/**
 * @brief Gets all points facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_points()
{
	MutexLocker lock(&env_mutex_);

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "points")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("points", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "phase")).c_str(), alloc);
				d.AddMember("phase", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "reason")).c_str(), alloc);
				d.AddMember("reason", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
				d.AddMember("team", json_string, alloc);
				json_string.SetInt((get_value<int64_t>(fact, "points")));
				d.AddMember("points", json_string, alloc);
				json_string.SetFloat((get_value<float>(fact, "game-time")));
				d.AddMember("game_time", json_string, alloc);

				//send it off
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type points");
			}
		}
		fact = fact->next();
	}
}

/**
 * @brief Gets all workpiece facts from CLIPS and pushes them to the send queue
 * 
 */
void
Data::log_push_workpiece_info()
{
	MutexLocker lock(&env_mutex_);

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "workpiece")) {
			try {
				rapidjson::Document d;
				d.SetObject();
				rapidjson::Document::AllocatorType &alloc = d.GetAllocator();
				//generic type information
				rapidjson::Value json_string;
				json_string.SetString("clips", alloc);
				d.AddMember("level", json_string, alloc);
				json_string.SetString("workpiece-info", alloc);
				d.AddMember("type", json_string, alloc);
				//value fields
				json_string.SetString((get_value<std::string>(fact, "at-machine")).c_str(), alloc);
				d.AddMember("at_machine", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
				d.AddMember("state", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "base-color")).c_str(), alloc);
				d.AddMember("base_color", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "cap-color")).c_str(), alloc);
				d.AddMember("cap_color", json_string, alloc);
				json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
				d.AddMember("team", json_string, alloc);
				json_string.SetFloat((get_value<int64_t>(fact, "id")));
				d.AddMember("id", json_string, alloc);
				json_string.SetFloat((get_value<int64_t>(fact, "order")));
				d.AddMember("order", json_string, alloc);
				json_string.SetFloat((get_value<float>(fact, "visible")));
				d.AddMember("visible", json_string, alloc);

				rapidjson::Value rings_array(rapidjson::kArrayType);
				rings_array.Reserve(get_values(fact, "ring-colors").size(), alloc);
				for (const auto &e : get_values(fact, "ring-colors")) {
					rapidjson::Value v;
					v.SetString(e, alloc);
					rings_array.PushBack(v, alloc);
				}
				d.AddMember("ring_colors", rings_array, alloc);

				//send it off
				log_push(d);
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type workpiece");
			}
		}
		fact = fact->next();
	}
}

} // namespace llsfrb::websocket