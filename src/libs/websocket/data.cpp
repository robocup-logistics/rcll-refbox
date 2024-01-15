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

#define BUFFER_SIZE 65536

namespace llsfrb::websocket {

/**
 * @brief Construct a new Data:: Data object
 *
 * @param logger_ logger to be used
 */
Data::Data(std::shared_ptr<Logger> logger, CLIPS::Environment *env, fawkes::Mutex &env_mutex)
: logger_(logger), env_mutex_(env_mutex)
{
	env_ = std::shared_ptr<CLIPS::Environment>(env);

	logger_->log_info("Websocket", "loading JSON schemas for command validation");

	std::string base_path      = std::string(SHAREDIR);
	std::string schema_names[] = {"confirm_delivery",
	                              "machine_add_base",
	                              "randomize_field",
	                              "set_gamephase",
	                              "set_gamestate",
	                              "set_machine_state",
	                              "set_order_delivered",
	                              "set_robot_maintenance",
	                              "set_teamname",
	                              "reset_machine_by_team",
	                              "add_points_team"};

	for (const std::string &schema_name : schema_names) {
		std::shared_ptr<rapidjson::SchemaDocument> sd =
		  load_schema(base_path + "/libs/websocket/message_schemas/" + schema_name + ".json");
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
std::shared_ptr<rapidjson::SchemaDocument>
Data::load_schema(std::string path)
{
	rapidjson::Document d;

	FILE *fp = std::fopen(path.c_str(), "rb");
	if (fp == 0) {
		logger_->log_error("Websocket", "couldn't load JSON schema file");
		return nullptr;
	}
	char                      readBuffer[BUFFER_SIZE];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	if (d.ParseStream(is).HasParseError()) {
		logger_->log_error("Websocket", "couldn't parse JSON schema file");
		return nullptr;
	}
	std::shared_ptr<rapidjson::SchemaDocument> schema =
	  std::make_shared<rapidjson::SchemaDocument>(d);
	std::fclose(fp);
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
Data::log_push_attention_message(std::string text,
                                 std::string team,
                                 int         time_to_display,
                                 float       game_time)
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
	json_string.SetInt(time_to_display);
	d.AddMember("time_to_display", json_string, alloc);
	json_string.SetFloat(game_time);
	d.AddMember("game_time", json_string, alloc);

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
 * @brief Gets specific machine-info fact from CLIPS and pushes it to the send queue
 *
 */
void
Data::log_push_machine_info(std::string name)
{
	MutexLocker          lock(&env_mutex_);
	std::vector<CLIPS::Fact::pointer> facts = {};
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "machine")) {
			try {
				if (get_value<std::string>(fact, "name") == name) {
					facts.push_back(fact);
				}
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type machine");
			}
		}
		fact = fact->next();
	}
	auto doc = pack_facts_to_doc("machine", facts, &Data::get_machine_info_fact<rapidjson::Value>);
	log_push(doc);

}

/**
 * @brief Gets a specific order-info fact from CLIPS and pushes it to the send queue
 *
 */
void
Data::log_push_order_info(int id)
{
	MutexLocker lock(&env_mutex_);

	std::vector<CLIPS::Fact::pointer> facts = {};
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "order")) {
			try {
				if (get_value<int64_t>(fact, "id") == id) {
					facts.push_back(fact);
				}
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type order");
			}
		}
		fact = fact->next();
	}
	auto doc = pack_facts_to_doc("order", facts, &Data::get_order_info_fact<rapidjson::Value>);
	log_push(doc);
}

/**
 * @brief
 *
 * @param delivery_id
 */
void
Data::log_push_order_info_via_delivery(int delivery_id)
{
	MutexLocker lock(&env_mutex_);

	std::vector<CLIPS::Fact::pointer> facts = {};
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "product-processed")) {
			try {
				if (get_value<int64_t>(fact, "id") == delivery_id) {
					CLIPS::Fact::pointer order = env_->get_facts();
					while (order) {
						if (match(fact, "order")) {
							if (get_value<int64_t>(fact, "order") == get_value<int64_t>(order, "id")) {
								facts.push_back(order);
							}
						}
						order = order->next();
					}
				}
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type order");
			}
		}
		fact = fact->next();
	}
	auto doc = pack_facts_to_doc("order", facts, &Data::get_order_info_fact<rapidjson::Value>);
	log_push(doc);
}

/**
 * @brief Gets a specific robot-info fact from CLIPS and pushes it to the send queue
 *
 */
void
Data::log_push_robot_info(int number, std::string name)
{
	MutexLocker lock(&env_mutex_);

	std::vector<CLIPS::Fact::pointer> facts = {};
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "robot")) {
			try {
				if (get_value<int64_t>(fact, "number") == number
				    && get_value<std::string>(fact, "name") == name) {
					facts.push_back(fact);
				}
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type robot");
			}
		}
		fact = fact->next();
	}
	auto doc = pack_facts_to_doc("robot", facts, &Data::get_robot_info_fact<rapidjson::Value>);
	log_push(doc);
}

/*
 * @brief Gets a specific agent-task fact from CLIPS and pushes it to the send queue
 *
 */
void
Data::log_push_agent_task_info(int tid, int rid)
{
	MutexLocker lock(&env_mutex_);

	std::vector<CLIPS::Fact::pointer> facts = {};
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "agent-task")) {
			try {
				if (get_value<int64_t>(fact, "task-id") == tid
				    && get_value<int64_t>(fact, "robot-id") == rid) {
					facts.push_back(fact);
				}
			} catch (Exception &e) {
				logger_->log_error("Websocket", "can't access value(s) of fact of type robot");
			}
		}
		fact = fact->next();
	}
	auto doc = pack_facts_to_doc("agent-task", facts, &Data::get_agent_task_info_fact<rapidjson::Value>);
	log_push(doc);
}

/**
 * @brief Gets the game-state fact from CLIPS and pushes it to the send queue
 *
 */
void
Data::log_push_game_state()
{
	auto doc = pack_facts_to_doc("gamestate", &Data::get_game_state_fact<rapidjson::Value>);
	log_push(doc);
}

/**
 * @brief Gets all ring-spec facts from CLIPS and pushes them to the send queue as an array
 *
 */
void
Data::log_push_ring_spec()
{
	log_push(on_connect_ring_spec());
}

/**
 * @brief Gets all points facts from CLIPS and pushes them to the send queue as an array
 *
 */
void
Data::log_push_points()
{
	log_push(on_connect_points());
}

/**
 * @brief Gets a specifc workpiece fact from CLIPS and pushes it to the send queue
 *
 */
void
Data::log_push_workpiece_info(int id)
{
	MutexLocker lock(&env_mutex_);
	std::vector<CLIPS::Fact::pointer> facts = {};
	// get machine facts pointers
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "workpiece") && get_value<int64_t>(fact, "id") == id) {
	        facts.push_back(fact);
	    }
	    fact = fact->next();
	}
	auto doc = pack_facts_to_doc("workpiece", facts, &Data::get_workpiece_info_fact<rapidjson::Value>);
	log_push(doc);
}

/**
 * @brief Gets all known teams facts from CLIPS and pushes them to the send queue as an array
 *
 */
void
Data::log_push_known_teams()
{
	log_push(on_connect_known_teams());
}

/**
 * @brief Create a string of a JSON array containing the data of all current known teams facts
 *
 * @return std::string
 */
std::string
Data::on_connect_known_teams()
{
	return on_connect_info("known-teams", &Data::get_known_teams_fact<rapidjson::Value>);
}

/**
 * @brief Create a string of a JSON array containing the data of all current workpiece info facts
 *
 * @return std::string
 */
std::string
Data::on_connect_workpiece_info()
{
	return on_connect_info("workpiece", &Data::get_workpiece_info_fact<rapidjson::Value>);
}

/**
 * @brief Create a string of a JSON array containing the data of all current robot info facts
 *
 * @return std::string
 */
std::string
Data::on_connect_robot_info()
{
	return on_connect_info("robot", &Data::get_robot_info_fact<rapidjson::Value>);
}

/**
 * @brief  Create a string of a JSON array containing the data of all current ring spec facts
 *
 * @return std::string
 */
std::string
Data::on_connect_ring_spec()
{
	return on_connect_info("ring-spec", &Data::get_ring_spec_fact<rapidjson::Value>);
}

/**
 * @brief Create a string of a JSON array containing the data of all current points facts
 *
 * @return std::string
 */
std::string
Data::on_connect_points()
{
	return on_connect_info("points", &Data::get_points_fact<rapidjson::Value>);
}

/**
 * @brief Create a string of a JSON array containing the data of all current order info facts
 *
 * @return std::string
 */
std::string
Data::on_connect_order_info()
{
	return on_connect_info("order", &Data::get_order_info_fact<rapidjson::Value>);
}

/**
 * @brief Create a string of a JSON object containing the count of existing orders
 *
 * @return std::string
 */
std::string
Data::on_connect_order_count()
{
	MutexLocker                       lock(&env_mutex_);
	std::vector<CLIPS::Fact::pointer> facts = {};

	//count order info pointers
	CLIPS::Fact::pointer fact    = env_->get_facts();
	int                  counter = 0;
	while (fact) {
		if (match(fact, "order")) {
			counter++;
		}
		fact = fact->next();
	}

	rapidjson::Document d;
	d.SetObject();
	rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

	rapidjson::Value json_string;
	json_string.SetString("clips", alloc);
	d.AddMember("level", json_string, alloc);
	json_string.SetString("order-count", alloc);
	d.AddMember("type", json_string, alloc);
	json_string.SetInt(counter);
	d.AddMember("count", json_string, alloc);

	rapidjson::StringBuffer                    buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);

	return buffer.GetString();
}

/**
 * @brief Create a string of a JSON array containing the data of all current machine info facts
 *
 * @return std::string
 */
std::string
Data::on_connect_machine_info()
{
	return on_connect_info("machine", &Data::get_machine_info_fact<rapidjson::Value>);
}

/**
 * @brief Pack a given set of facts to a document
 *
 * @param tmpl_name
 * @param get_info_fact
 * @return doc with facts
 */
rapidjson::Document
Data::pack_facts_to_doc(std::string tmpl_name, std::vector<CLIPS::Fact::pointer> &facts,
                      void (Data::*get_info_fact)(rapidjson::Value *,
                                                  rapidjson::Document::AllocatorType &,
                                                  CLIPS::Fact::pointer))
{
	MutexLocker         lock(&env_mutex_);
	rapidjson::Document root;
	root.SetObject();

	// Set 'level' and 'type'
	root.AddMember("level", "clips", root.GetAllocator());
	root.AddMember("type", tmpl_name, root.GetAllocator());
	rapidjson::Value contentArray(rapidjson::kArrayType);

	// get facts and pack into json array
	for (CLIPS::Fact::pointer fact : facts) {
	    try {
	        rapidjson::Value factObject;
	        factObject.SetObject();
	        (this->*get_info_fact)(&factObject, root.GetAllocator(), fact);
	        // add to 'content' array
	        contentArray.PushBack(factObject, root.GetAllocator());
	    } catch (Exception &e) {
	        logger_->log_error("Websocket", "can't access value(s) of fact, omitting");
	    }
	}
	
	// add 'content' array to the root object
	root.AddMember("content", contentArray, root.GetAllocator());
	return root;
}

/**
 * @brief Prepare a message that contains all facts of a given template name
 *
 * @param tmpl_name
 * @param get_info_fact
 * @return doc with facts
 */
rapidjson::Document
Data::pack_facts_to_doc(std::string tmpl_name,
                      void (Data::*get_info_fact)(rapidjson::Value *,
                                                  rapidjson::Document::AllocatorType &,
                                                  CLIPS::Fact::pointer))
{
	MutexLocker         lock(&env_mutex_);
	rapidjson::Document root;
	root.SetObject();

	// Set 'level' and 'type'
	root.AddMember("level", "clips", root.GetAllocator());
	root.AddMember("type", tmpl_name, root.GetAllocator());
	rapidjson::Value contentArray(rapidjson::kArrayType);

	std::vector<CLIPS::Fact::pointer> facts = {};
	// get machine facts pointers
	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
	    if (match(fact, tmpl_name)) {
	        facts.push_back(fact);
	    }
	    fact = fact->next();
	}

	// get facts and pack into json array
	for (CLIPS::Fact::pointer fact : facts) {
	    try {
	        rapidjson::Value factObject;
	        factObject.SetObject();
	        (this->*get_info_fact)(&factObject, root.GetAllocator(), fact);
	        // add to 'content' array
	        contentArray.PushBack(factObject, root.GetAllocator());
	    } catch (Exception &e) {
	        logger_->log_error("Websocket", "can't access value(s) of fact, omitting");
	    }
	}
	
	// add 'content' array to the root object
    if (contentArray.Size() > 1) {
        root.AddMember("content", contentArray, root.GetAllocator());
    } else if (contentArray.Size() == 1) {
        // If there is only one element, add it directly without the array
        root.AddMember("content", contentArray[0], root.GetAllocator());
    }
	return root;
}

/**
 * @brief Prepare a message that contains all facts of a given template name
 *
 * @param tmpl_name
 * @param get_info_fact
 * @return std::string
 */
std::string
Data::on_connect_info(std::string tmpl_name,
                      void (Data::*get_info_fact)(rapidjson::Value *,
                                                  rapidjson::Document::AllocatorType &,
                                                  CLIPS::Fact::pointer))
{
	auto doc = pack_facts_to_doc(tmpl_name,get_info_fact);
	//write to string and return
	rapidjson::StringBuffer                    buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	return buffer.GetString();
}

/**
 * @brief Gets data of the saved known-teams on the refbox side to support user input
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_known_teams_fact(T                                  *o,
                           rapidjson::Document::AllocatorType &alloc,
                           CLIPS::Fact::pointer                fact)
{
	//value fields
	rapidjson::Value teams_array(rapidjson::kArrayType);
	teams_array.Reserve(get_values(fact, "").size(), alloc);
	for (const auto &e : get_values(fact, "")) {
		rapidjson::Value v;
		v.SetString(e, alloc);
		teams_array.PushBack(v, alloc);
	}
	(*o).AddMember("known_teams", teams_array, alloc);
}

/**
 * @brief Gets data of a machine-info fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_machine_info_fact(T                                  *o,
                            rapidjson::Document::AllocatorType &alloc,
                            CLIPS::Fact::pointer                fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "name")).c_str(), alloc);
	(*o).AddMember("name", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
	(*o).AddMember("team", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "mtype")).c_str(), alloc);
	(*o).AddMember("mtype", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
	(*o).AddMember("state", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "zone")).c_str(), alloc);
	(*o).AddMember("zone", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "rotation")));
	(*o).AddMember("rotation", json_string, alloc);

	CLIPS::Fact::pointer meta_fact = env_->get_facts();
	while (meta_fact) {
		if (match(meta_fact, "cs-meta")
		    && get_value<std::string>(fact, "name") == get_value<std::string>(meta_fact, "name")) {
			json_string.SetString((get_value<std::string>(meta_fact, "operation-mode")).c_str(), alloc);
			(*o).AddMember("operation_mode", json_string, alloc);
			json_string.SetBool((get_value<bool>(meta_fact, "has-retrieved")));
			(*o).AddMember("has_retrieved", json_string, alloc);
			break;
		}
		if (match(meta_fact, "rs-meta")
		    && get_value<std::string>(fact, "name") == get_value<std::string>(meta_fact, "name")) {
			json_string.SetString((get_value<std::string>(meta_fact, "current-ring-color")).c_str(), alloc);
			(*o).AddMember("current_ring_color", json_string, alloc);
			rapidjson::Value ring_array(rapidjson::kArrayType);
			ring_array.Reserve(get_values(meta_fact, "available-colors").size(), alloc);
			for (const auto &e : get_values(meta_fact, "available-colors")) {
				rapidjson::Value v;
				v.SetString(e, alloc);
				ring_array.PushBack(v, alloc);
			}
			(*o).AddMember("available_colors", ring_array, alloc);
			json_string.SetInt((get_value<int64_t>(meta_fact, "bases-added")));
			(*o).AddMember("bases_added", json_string, alloc);
			json_string.SetInt((get_value<int64_t>(meta_fact, "bases-used")));
			(*o).AddMember("bases_used", json_string, alloc);
		}
		if (match(meta_fact, "bs-meta")
		    && get_value<std::string>(fact, "name") == get_value<std::string>(meta_fact, "name")) {
			json_string.SetString((get_value<std::string>(meta_fact, "current-side")).c_str(), alloc);
			(*o).AddMember("current_side", json_string, alloc);
			json_string.SetString((get_value<std::string>(meta_fact, "current-base-color")).c_str(), alloc);
			(*o).AddMember("current_base_color", json_string, alloc);
			break;
		}
		if (match(meta_fact, "ds-meta")
		    && get_value<std::string>(fact, "name") == get_value<std::string>(meta_fact, "name")) {
			json_string.SetInt((get_value<int64_t>(meta_fact, "order-id")));
			(*o).AddMember("order_id", json_string, alloc);
			break;
		}
		meta_fact = meta_fact->next();
	}
	CLIPS::Fact::pointer lights_fact = env_->get_facts();
	while (lights_fact) {
		if (match(lights_fact, "machine-lights")
		    && get_value<std::string>(fact, "name") == get_value<std::string>(lights_fact, "name")) {
			rapidjson::Value lights_array(rapidjson::kArrayType);
			lights_array.Reserve(get_values(fact, "actual-lights").size(), alloc);
			for (const auto &e : get_values(fact, "actual-lights")) {
				rapidjson::Value v;
				v.SetString(e, alloc);
				lights_array.PushBack(v, alloc);
			}
			(*o).AddMember("actual_lights", lights_array, alloc);
			break;
		}
		lights_fact = lights_fact->next();
	}
}

/**
 * @brief Gets data of a order-info fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_order_info_fact(T                                  *o,
                          rapidjson::Document::AllocatorType &alloc,
                          CLIPS::Fact::pointer                fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "complexity")).c_str(), alloc);
	(*o).AddMember("complexity", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "base-color")).c_str(), alloc);
	(*o).AddMember("base_color", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "cap-color")).c_str(), alloc);
	(*o).AddMember("cap_color", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "id")));
	(*o).AddMember("id", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "quantity-requested")));
	(*o).AddMember("quantity_requested", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "delivery-gate")));
	(*o).AddMember("delivery_gate", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "competitive")));
	(*o).AddMember("competitive", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "activate-at")));
	(*o).AddMember("activate_at", json_string, alloc);
	rapidjson::Value delivery_array(rapidjson::kArrayType);
	delivery_array.Reserve(fact->slot_value("delivery-period").size(), alloc);
	for (auto value : fact->slot_value("delivery-period")) {
		rapidjson::Value v;
		v.SetInt(value.as_integer());
		delivery_array.PushBack(v, alloc);
	}
	(*o).AddMember("delivery_period", delivery_array, alloc);

	rapidjson::Value quantity_array(rapidjson::kArrayType);
	quantity_array.Reserve(fact->slot_value("quantity-delivered").size(), alloc);
	for (auto value : fact->slot_value("quantity-delivered")) {
		rapidjson::Value v;
		v.SetInt(value.as_integer());
		quantity_array.PushBack(v, alloc);
	}
	(*o).AddMember("quantity_delivered", quantity_array, alloc);

	rapidjson::Value ring_array(rapidjson::kArrayType);
	ring_array.Reserve(get_values(fact, "ring-colors").size(), alloc);
	for (const auto &e : get_values(fact, "ring-colors")) {
		rapidjson::Value v;
		v.SetString(e, alloc);
		ring_array.PushBack(v, alloc);
	}
	(*o).AddMember("ring_colors", ring_array, alloc);

	(*o).AddMember("unconfirmed_deliveries",
	               get_unconfirmed_delivery_fact(alloc, get_value<int64_t>(fact, "id")),
	               alloc);
}

/**
 * @brief Creates a rapidjson array of unconfirmed delivery objects for the given order id
 *
 * @param alloc
 * @param id order id to get the correct unconfirmed delivery
 * @return rapidjson::Value
 */
rapidjson::Value
Data::get_unconfirmed_delivery_fact(rapidjson::Document::AllocatorType &alloc, int64_t id)
{
	rapidjson::Value unconfirmed_delivery(rapidjson::kArrayType);
	rapidjson::Value json_string;

	CLIPS::Fact::pointer delivery = env_->get_facts();
	while (delivery) {
		if (match(delivery, "product-processed")) {
			if (get_value<std::string>(delivery, "confirmed") == "FALSE"
			    && get_value<int64_t>(delivery, "order") == id
			    && get_value<std::string>(delivery, "mtype") == "DS") {
				CLIPS::Fact::pointer referee_confirmation = env_->get_facts();
				while (referee_confirmation) {
					if (match(referee_confirmation, "referee-confirmation")
					    && get_value<int64_t>(delivery, "id")
					         == get_value<int64_t>(referee_confirmation, "process-id")
					    && get_value<std::string>(referee_confirmation, "state") == "REQUIRED") {
						rapidjson::Value o;
						o.SetObject();
						json_string.SetInt((get_value<int64_t>(delivery, "id")));
						o.AddMember("delivery_id", json_string, alloc);
						json_string.SetString((get_value<std::string>(delivery, "team")).c_str(), alloc);
						o.AddMember("team", json_string, alloc);
						json_string.SetFloat((get_value<float>(delivery, "game-time")));
						o.AddMember("game_time", json_string, alloc);

						unconfirmed_delivery.PushBack(o, alloc);
					}
					referee_confirmation = referee_confirmation->next();
				}
			}
		}
		delivery = delivery->next();
	}

	return unconfirmed_delivery;
}

/**
 * @brief Gets data of a robot-info fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_robot_info_fact(T                                  *o,
                          rapidjson::Document::AllocatorType &alloc,
                          CLIPS::Fact::pointer                fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
	(*o).AddMember("state", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
	(*o).AddMember("team", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "team-color")).c_str(), alloc);
	(*o).AddMember("team_color", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "name")).c_str(), alloc);
	(*o).AddMember("name", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "host")).c_str(), alloc);
	(*o).AddMember("host", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "number")));
	(*o).AddMember("number", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "port")));
	(*o).AddMember("port", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "warning-sent")));
	(*o).AddMember("warning_sent", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "maintenance-start-time")));
	(*o).AddMember("maintenance_start_time", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "maintenance-cycles")));
	(*o).AddMember("maintenance_cycles", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "has-pose")));
	(*o).AddMember("has_pose", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "maintenance-warning-sent")));
	(*o).AddMember("maintenance_warning_sent", json_string, alloc);

	rapidjson::Value last_seen_array(rapidjson::kArrayType);
	last_seen_array.Reserve(fact->slot_value("last-seen").size(), alloc);
	for (auto value : fact->slot_value("last-seen")) {
		rapidjson::Value v;
		v.SetFloat(value.as_float());
		last_seen_array.PushBack(v, alloc);
	}
	(*o).AddMember("last_seen", last_seen_array, alloc);

	rapidjson::Value pose_array(rapidjson::kArrayType);
	pose_array.Reserve(fact->slot_value("pose").size(), alloc);
	for (auto value : fact->slot_value("pose")) {
		rapidjson::Value v;
		v.SetFloat(value.as_float());
		pose_array.PushBack(v, alloc);
	}
	(*o).AddMember("pose", pose_array, alloc);
}

/**
 * @brief Gets data from an agent-task fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_agent_task_info_fact(T                                  *o,
                               rapidjson::Document::AllocatorType &alloc,
                               CLIPS::Fact::pointer                fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "task-type")).c_str(), alloc);
	(*o).AddMember("task_type", json_string, alloc);

	rapidjson::Value task_params_array(rapidjson::kArrayType);
	task_params_array.Reserve(get_values(fact, "task-parameters").size(), alloc);
	for (const auto &e : get_values(fact, "task-parameters")) {
		rapidjson::Value v;
		v.SetString(e, alloc);
		task_params_array.PushBack(v, alloc);
	}
	(*o).AddMember("task_parameters", task_params_array, alloc);

	json_string.SetInt(get_value<int64_t>(fact, "task-id"));
	(*o).AddMember("task_id", json_string, alloc);
	json_string.SetInt(get_value<int64_t>(fact, "robot-id"));
	(*o).AddMember("robot_id", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "team-color")).c_str(), alloc);
	(*o).AddMember("team_color", json_string, alloc);
	json_string.SetFloat(get_value<float>(fact, "start-time"));
	(*o).AddMember("start_time", json_string, alloc);
	json_string.SetFloat(get_value<float>(fact, "end-time"));
	(*o).AddMember("end_time", json_string, alloc);
	json_string.SetInt(get_value<int64_t>(fact, "order-id"));
	(*o).AddMember("order_id", json_string, alloc);
	json_string.SetBool(get_value<bool>(fact, "processed"));
	(*o).AddMember("processed", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "workpiece-name")).c_str(), alloc);
	(*o).AddMember("workpiece_name", json_string, alloc);
	json_string.SetBool(get_value<bool>(fact, "unknown-action"));
	(*o).AddMember("unknown_action", json_string, alloc);
	json_string.SetBool(get_value<bool>(fact, "successful"));
	(*o).AddMember("successful", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "base-color")).c_str(), alloc);
	(*o).AddMember("base_color", json_string, alloc);

	rapidjson::Value ring_colors_array(rapidjson::kArrayType);
	ring_colors_array.Reserve(get_values(fact, "ring-color").size(), alloc);
	for (const auto &e : get_values(fact, "ring-color")) {
		rapidjson::Value v;
		v.SetString(e, alloc);
		ring_colors_array.PushBack(v, alloc);
	}
	(*o).AddMember("ring_color", ring_colors_array, alloc);

	json_string.SetString((get_value<std::string>(fact, "cap-color")).c_str(), alloc);
	(*o).AddMember("cap_color", json_string, alloc);
}

/**
 * @brief Gets data of a gamestate fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_game_state_fact(T                                  *o,
                          rapidjson::Document::AllocatorType &alloc,
                          CLIPS::Fact::pointer                fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
	(*o).AddMember("state", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "phase")).c_str(), alloc);
	(*o).AddMember("phase", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "prev-phase")).c_str(), alloc);
	(*o).AddMember("prev_phase", json_string, alloc);
	json_string.SetFloat((get_value<float>(fact, "game-time")));
	(*o).AddMember("game_time", json_string, alloc);
	json_string.SetFloat((get_value<float>(fact, "cont-time")));
	(*o).AddMember("cont_time", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "over-time")));
	(*o).AddMember("over_time", json_string, alloc);
	json_string.SetString((get_values(fact, "teams")[0]).c_str(), alloc);
	(*o).AddMember("cyan", json_string, alloc);
	json_string.SetString((get_values(fact, "teams")[1]).c_str(), alloc);
	(*o).AddMember("magenta", json_string, alloc);
	json_string.SetInt(fact->slot_value("points")[0].as_integer());
	(*o).AddMember("points_cyan", json_string, alloc);
	json_string.SetInt(fact->slot_value("points")[1].as_integer());
	(*o).AddMember("points_magenta", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "field-height")));
	(*o).AddMember("field_height", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "field-width")));
	(*o).AddMember("field_width", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "field-mirrored")));
	(*o).AddMember("field_mirrored", json_string, alloc);
}

/**
 * @brief Gets data of a ring-spec fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_ring_spec_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "color")).c_str(), alloc);
	(*o).AddMember("color", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "req-bases")));
	(*o).AddMember("req_bases", json_string, alloc);
}

/**
 * @brief Gets data of a points fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_points_fact(T *o, rapidjson::Document::AllocatorType &alloc, CLIPS::Fact::pointer fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "phase")).c_str(), alloc);
	(*o).AddMember("phase", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "reason")).c_str(), alloc);
	(*o).AddMember("reason", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
	(*o).AddMember("team", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "order")));
	(*o).AddMember("order", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "points")));
	(*o).AddMember("points", json_string, alloc);
	json_string.SetFloat((get_value<float>(fact, "game-time")));
	(*o).AddMember("game_time", json_string, alloc);
}

/**
 * @brief Gets data of a workpiece-info fact and packs into into a rapidjson object
 *
 * @tparam T
 * @param o
 * @param alloc
 * @param fact
 */
template <class T>
void
Data::get_workpiece_info_fact(T                                  *o,
                              rapidjson::Document::AllocatorType &alloc,
                              CLIPS::Fact::pointer                fact)
{
	rapidjson::Value json_string;
	//value fields
	json_string.SetString((get_value<std::string>(fact, "name")).c_str(), alloc);
	(*o).AddMember("name", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "at-machine")).c_str(), alloc);
	(*o).AddMember("at_machine", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "at-side")).c_str(), alloc);
	(*o).AddMember("at_side", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "holding")));
	(*o).AddMember("holding", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "robot-holding")));
	(*o).AddMember("robot_holding", json_string, alloc);
	json_string.SetBool((get_value<bool>(fact, "unknown-action")));
	(*o).AddMember("unknown_action", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "state")).c_str(), alloc);
	(*o).AddMember("state", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "base-color")).c_str(), alloc);
	(*o).AddMember("base_color", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "cap-color")).c_str(), alloc);
	(*o).AddMember("cap_color", json_string, alloc);
	json_string.SetString((get_value<std::string>(fact, "team")).c_str(), alloc);
	(*o).AddMember("team", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "id")));
	(*o).AddMember("id", json_string, alloc);
	json_string.SetInt((get_value<int64_t>(fact, "order")));
	(*o).AddMember("order", json_string, alloc);
	json_string.SetFloat((get_value<float>(fact, "visible")));
	(*o).AddMember("visible", json_string, alloc);

	rapidjson::Value rings_array(rapidjson::kArrayType);
	rings_array.Reserve(get_values(fact, "ring-colors").size(), alloc);
	for (const auto &e : get_values(fact, "ring-colors")) {
		rapidjson::Value v;
		v.SetString(e, alloc);
		rings_array.PushBack(v, alloc);
	}
	(*o).AddMember("ring_colors", rings_array, alloc);
}

/**
 * @brief Get the current phase of the game as a string
 *
 * @return std::string
 */
std::string
Data::get_gamephase()
{
	MutexLocker                       lock(&env_mutex_);
	std::vector<CLIPS::Fact::pointer> facts = {};

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "gamestate")) {
			return get_value<std::string>(fact, "phase");
		}
		fact = fact->next();
	}
	return NULL;
}

/**
 * @brief Get the current state of the game as a string
 *
 * @return std::string
 */
std::string
Data::get_gamestate()
{
	MutexLocker                       lock(&env_mutex_);
	std::vector<CLIPS::Fact::pointer> facts = {};

	CLIPS::Fact::pointer fact = env_->get_facts();
	while (fact) {
		if (match(fact, "gamestate")) {
			return get_value<std::string>(fact, "state");
		}
		fact = fact->next();
	}
	return NULL;
}

} // namespace llsfrb::websocket
