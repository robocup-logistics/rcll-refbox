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

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string>
#include <vector>
#include <queue>

#include "data.h"

namespace llsfrb::websocket
{

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
std::string Data::log_pop()
{
    const std::lock_guard<std::mutex> lock(log_mu);
    std::string log = logs.front();
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
void Data::log_push(std::string log)
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
void Data::log_push(rapidjson::Document &d)
{
    const std::lock_guard<std::mutex> lock(log_mu);
    rapidjson::StringBuffer buffer;
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
bool Data::log_empty() { return logs.empty(); }

/**
 * @brief blocks calling thread until log queue is not-empty
 * 
 */
void Data::log_wait()
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
void Data::clients_add(std::shared_ptr<Client> client)
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
void Data::clients_send_all(std::string msg)
{
    const std::lock_guard<std::mutex> lock(cli_mu);

    std::vector<std::shared_ptr<Client>> unfailed_clients;

    for (auto const &client : clients)
    {
        if (client->send(msg))
        {
            unfailed_clients.push_back(client);
        }
        else
        {
            logger_->log_info("Websocket", "client disconnected");
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
void Data::clients_send_all(rapidjson::Document &d)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    clients_send_all(buffer.GetString());
}

} // namespace llsfrb::websocket