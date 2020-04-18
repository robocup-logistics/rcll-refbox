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
                   Logger *                                                      logger)
: socket(socket)
{
	logger_ = logger;
	socket->accept();
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
ClientS::ClientS(std::shared_ptr<tcp::socket> socket, Logger *logger) : socket(socket)
{
	logger_  = logger;
	client_t = std::thread(&Client::receive_thread, this);
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
			if (!msgs.IsObject()) {
				logger_->log_error("Websocket", "non JSON message received, won't process");
			}
			//logger_->log_warn("Websocket", msgs["message"].GetString());
		} catch (std::exception &e) {
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