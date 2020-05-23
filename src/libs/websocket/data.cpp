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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <condition_variable>
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
Data::Data(Logger *logger_) : logger_(logger_)
{
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
 * @brief log the facts of the gamestate 
 * 
 *  Take the gamestate facts, build a JSON string and push it in the log queue. 
 * 
 * @param time 
 * @param phase 
 * @param prevphase 
 * @param team_cyan 
 * @param team_magenta 
 */
void
Data::log_push_fact_gamestate(std::string time,
                              std::string state,
                              std::string phase,
                              std::string prevphase,
                              std::string team_cyan,
                              std::string team_magenta)
{
	rapidjson::Document d;
	d.SetObject();
	rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

	rapidjson::Value json_string;
	json_string.SetString("clips", alloc);
	d.AddMember("level", json_string, alloc);
	json_string.SetString("gamestate", alloc);
	d.AddMember("type", json_string, alloc);
	json_string.SetString((state).c_str(), alloc);
	d.AddMember("state", json_string, alloc);
	json_string.SetString((phase).c_str(), alloc);
	d.AddMember("gamephase", json_string, alloc);
	json_string.SetString((prevphase).c_str(), alloc);
	d.AddMember("prevphase", json_string, alloc);
	json_string.SetString((time).c_str(), alloc);
	d.AddMember("gametime", json_string, alloc);
	json_string.SetString((team_cyan).c_str(), alloc);
	d.AddMember("cyan", json_string, alloc);
	json_string.SetString((team_magenta).c_str(), alloc);
	d.AddMember("magenta", json_string, alloc);

	log_push(d);
}

/**
 * @brief log the points of the teams
 * 
 * Called from CLIPS environment when points for the teams get udpated, forward those to the clients 
 * 
 * @param points_cyan 
 * @param points_magenta 
 */
void
Data::log_push_fact_gamepoints(std::string points_cyan, std::string points_magenta)
{
	rapidjson::Document d;
	d.SetObject();
	rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

	rapidjson::Value json_string;
	json_string.SetString("clips", alloc);
	d.AddMember("level", json_string, alloc);
	json_string.SetString("gamepoints", alloc);
	d.AddMember("type", json_string, alloc);
	json_string.SetString((points_cyan).c_str(), alloc);
	d.AddMember("points_cyan", json_string, alloc);
	json_string.SetString((points_magenta).c_str(), alloc);
	d.AddMember("points_magenta", json_string, alloc);

	log_push(d);
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

} // namespace llsfrb::websocket