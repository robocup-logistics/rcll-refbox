// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  server.cpp - class to provide server functionality to handle incoming connections
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

#include "server.h"

#include "client.h"
#include "data.h"

#include <sys/socket.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

namespace rcll::websocket {

/**
 * @brief Construct a new Server::Server object
 *
 *  Constructs a new Server object and assigns the used backend data pointer.
 *
 * @param data_ptr pointer to Data object that is used for this session
 * @param logger_ logger used by the backend
 */
Server::Server(std::shared_ptr<Data>    data,
               std::shared_ptr<Logger>  logger,
               boost::asio::io_service &io_service,
               uint                     port,
               bool                     ws_mode,
               bool                     allow_control_all)
: data_(data),
  logger_(logger),
  socket_(io_service),
  acceptor_(io_service, tcp::endpoint(tcp::v4(), port_)),
  ws_mode_(ws_mode),
  allow_control_all_(allow_control_all)
{
}

void
Server::shutdown()
{
	shutdown_ = true;
	acceptor_.close();
}

void
Server::handle_accept(const boost::system::error_code &error, boost::asio::ip::tcp::socket &socket)
{
	if (!error) {
		//client can send control command if allow_control_all_ is set or it is the localhost
		bool client_can_send =
		  (allow_control_all_ || socket.remote_endpoint().address().to_string() == "127.0.0.1");

		try {
			if (ws_mode_) {
				// websocket approach
				std::shared_ptr<boost::beast::websocket::stream<tcp::socket>> web_socket =
				  std::make_shared<boost::beast::websocket::stream<tcp::socket>>(std::move(socket));
				std::shared_ptr<Client> client =
				  std::make_shared<ClientWS>(web_socket, logger_, data_, client_can_send);
				data_->clients_add(client);
			} else {
				// socket approach
				std::shared_ptr<Client> client = std::make_shared<ClientS>(
				  std::make_shared<tcp::socket>(std::move(socket)), logger_, data_, client_can_send);
				data_->clients_add(client);
			}

			logger_->log_info("Websocket", "new client connected");
		} catch (const std::exception &e) {
			// Handle the exception, log an error message, etc.
			logger_->log_error("Websocket", "Failed to accept client connection: %s", e.what());
		}
	} else {
		logger_->log_warn("Websocket", "Connection to Client failed");
	}
}

void
Server::do_accept()
{
	boost::asio::ip::tcp::endpoint endpoint = tcp::endpoint(tcp::v4(), port_);
	acceptor_.async_accept(endpoint,
	                       [&](const boost::system::error_code &error,
	                           boost::asio::ip::tcp::socket     peer) {
		                       if (!error)
			                       handle_accept(error, peer);
		                       do_accept();
	                       });
}

/**
 * @brief Runs the Socket/Websocket Server
 *  This operator runs the socket or websocket server (currently based on WS_MODE predef).
 *  It launches an endless accepting loop, that accepts new connections and creates the
 *  necessary objects required by the backend to work with them.
 *
 */
void
Server::operator()()
{
	do_accept();
}

} // namespace rcll::websocket
