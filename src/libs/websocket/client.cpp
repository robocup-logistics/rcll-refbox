/***************************************************************************
 *  client.cpp - class to manage connected frontend clients
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

#include "client.h"

#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <sys/socket.h>

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>

using boost::asio::ip::tcp;

namespace llsfrb::websocket {

/**
 * @brief Construct a new ClientWS::ClientWS object
 * 
 * @param socket Established WebSocket socket shared pointer user for this client
 */
ClientWS::ClientWS(std::shared_ptr<boost::beast::websocket::stream<tcp::socket>> socket,
                   Logger *                                                      logger,
                   Data *                                                        data,
                   bool                                                          can_send)
: socket(socket)
{
	logger_   = logger;
	data_     = data;
	can_send_ = can_send;
	socket->accept();
	if(can_send)
		client_t = std::thread(&Client::receive_thread, this);
	logger_->log_info("Websocket", "client receive thread started");
}

/**
 * @brief Destroy the ClientWS:: ClientWS object
 * 
 */
ClientWS::~ClientWS()
{
	disconnect();
	client_t.join();
}

/**
 * @brief Send string message to client 
 * 
 *  Thread-safe WebSocket send function, sends a given string message with a trailing newline to the client. 
 * 
 * @param msg 
 * @return true send successful
 * @return false send unsuccessful (connection probably broken)
 * @param logger_ Logger instance to be used 
 */
bool
ClientWS::send(std::string msg)
{
	const std::lock_guard<std::mutex> lock(wr_mu);

	boost::system::error_code error;
	socket->write(boost::asio::buffer(msg + "\n"), error);
	if (error && error != boost::asio::error::eof) {
		return false;
	}

	return true;
}

/**
 * @brief Blocking receive function, returns received message
 * 
 *  Function waits for incoming string message and returns it once it is received.  
 * 
 * @return std::string received message
 */
std::string
ClientWS::read()
{
	const std::lock_guard<std::mutex> lock(rd_mu);

	boost::asio::streambuf buf;
	socket->read(buf);
	std::string data = boost::asio::buffer_cast<const char *>(buf.data());

	return data;
}

/**
 * @brief WebSocket implementation for close
 * 
 */
void
ClientWS::close()
{
	socket->next_layer().close();
}

/**
 * @brief Construct a new ClientS::ClientS object
 * 
 * @param socket TCP socket over which client communication happens
 * @param logger_ Logger instance to be used 
 */
ClientS::ClientS(std::shared_ptr<tcp::socket> socket, Logger *logger, Data *data, bool can_send)
: socket(socket)
{
	logger_   = logger;
	data_     = data;
	if(can_send)
		client_t  = std::thread(&Client::receive_thread, this);
	logger_->log_info("Websocket", "TCP-socket client receive thread started");
}

/**
 * @brief Destroy the ClientS::ClientS object
 * 
 */
ClientS::~ClientS()
{
	disconnect();
	client_t.join();
}

/**
 * @brief Send string message to client 
 * 
 *  Thread-safe TCP socket send function, sends a given string message with a trailing newline to the client. 
 * 
 * @param msg message to be sent
 * @return true send was successful
 * @return false send was unsuccessful
 */
bool
ClientS::send(std::string msg)
{
	const std::lock_guard<std::mutex> lock(wr_mu);

	boost::system::error_code error;
	boost::asio::write(*socket, boost::asio::buffer(msg + "\n"), error);
	if (error && error != boost::asio::error::eof) {
		return false;
	}
	return true;
}

/**
 * @brief Blocking receive function, returns received message
 * 
 *  Function waits for incoming string message and returns it once it is received. 
 * 
 * @return std::string received message
 */
std::string
ClientS::read()
{
	const std::lock_guard<std::mutex> lock(rd_mu);

	boost::asio::streambuf buf;
	boost::asio::read_until(*socket, buf, "\n");
	std::string data = boost::asio::buffer_cast<const char *>(buf.data());

	return data;
}

/**
 * @brief TCP-Socket implementation for close
 * 
 */
void
ClientS::close()
{
	socket->close();
}

/**
 * @brief Handles incoming message requests
 * 
 */
void
Client::receive_thread()
{
	rapidjson::Document msgs;

	while (active) {
		try {
			std::string input = read();
			msgs.Parse(input.c_str());

			//check incoming message type and call corresponding CLIPS function
			if (!msgs.IsObject()) {
				logger_->log_error("Websocket", "non JSON message received, won't process");
			} else if (!can_send_) {
				logger_->log_error("Websocket", "non localhost client tried to send command");
			} else if (msgs.HasMember("command")) {
				std::string                command = msgs["command"].GetString();
				rapidjson::SchemaValidator validator(*(data_->command_schema_map[command]));
				if (!msgs.Accept(validator)) {
					logger_->log_error("Websocket", "input JSON is invalid!");
				} else {
					if (strcmp(msgs["command"].GetString(), "set_gamestate") == 0) {
						data_->clips_set_gamestate(msgs["state"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_gamephase") == 0) {
						data_->clips_set_gamephase(msgs["phase"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "randomize_field") == 0) {
						data_->clips_randomize_field();
					}
					if (strcmp(msgs["command"].GetString(), "set_teamname") == 0) {
						data_->clips_set_teamname(msgs["color"].GetString(), msgs["name"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "confirm_delivery") == 0) {
						data_->clips_confirm_delivery(msgs["delivery_id"].GetInt(),
						                              msgs["correctness"].GetBool(),
						                              msgs["order_id"].GetInt(),
						                              msgs["color"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_order_delivered") == 0) {
						data_->clips_set_order_delivered(msgs["color"].GetString(), msgs["order_id"].GetInt());
					}
					if (strcmp(msgs["command"].GetString(), "set_machine_state") == 0) {
						data_->clips_production_set_machine_state(msgs["mname"].GetString(),
						                                          msgs["state"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "machine_add_base") == 0) {
						data_->clips_production_machine_add_base(msgs["mname"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_robot_maintenance") == 0) {
						data_->clips_robot_set_robot_maintenance(msgs["robot_number"].GetInt(),
						                                         msgs["team_color"].GetString(),
						                                         msgs["maintenance"].GetBool());
					}
				}
			} else {
				logger_->log_error("Websocket", "malformed message received, won't be processed");
			}

		} catch (std::exception &e) {
			disconnect();
		} catch (...) {
			disconnect();
		}
	}
}

/**
 * @brief Disconnects client by closing connection and stopping receive thread
 * 
 */
void
Client::disconnect()
{
	if (active) {
		active = false;
		close();
		logger_->log_info("Websocket", "client disconnected");
	}
}

} // namespace llsfrb::websocket