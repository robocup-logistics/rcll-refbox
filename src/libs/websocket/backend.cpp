// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  backend.cpp - class to provide backend functionality for webfrontend
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

#include "backend.h"

#include "server.h"

#include <iostream>
#include <string>
#include <thread>

namespace rcll::websocket {

/**
 * @brief Construct a new Backend::Backend object
 *
 *  Construct a new Backend object with assigned data and server.
 *
 * @param logger_ logger used by the backend
 * @param env_ clips environment for callbacks
 * @param env_mutex mutext to coordinate env access
 * @param io_service io_service to use
 * @param port tcp port of the websocket server
 * @param ws_mode true if websocket only mode is activated
 * @param allow_control_all if this is set, devices with not local host ip addresses can send control commands
 */
Backend::Backend(Logger                             *logger,
                 std::shared_ptr<CLIPS::Environment> env,
                 fawkes::Mutex                      &env_mutex,
                 boost::asio::io_service            &io_service,
                 uint                                port,
                 bool                                ws_mode,
                 bool                                allow_control_all)
: logger_(logger),
  data_(std::make_shared<Data>(logger_, env, env_mutex)),
  server_(data_, logger_, io_service, port, ws_mode, allow_control_all)
{
	// launch server thread
	server_t_ = std::thread(std::bind(&Server::operator(), &server_));
	logger_->log_info("Websocket", "(web-)socket-server started");
	// launch backend thread
	backend_t_ = std::thread(&Backend::operator(), this);
	logger_->log_info("Websocket", "backend started");
}

Backend::~Backend()
{
	shutdown_ = true;
	data_->shutdown();
	server_.shutdown();
	if (backend_t_.joinable()) {
		backend_t_.join();
	}
	if (server_t_.joinable()) {
		server_t_.join();
	}
	data_.reset();
}

/**
 * @brief Operator runs the backend in the current thread
 *
 *  This operator runs the webfrontend backend; works
 *  through the message queue.
 *
 */
void
Backend::operator()()
{
	// message queue handler -> consumer
	bool msgs_running = true;
	while (!shutdown_ && msgs_running) {
		// block until new message available
		data_->log_wait();
		if (shutdown_) {
			break;
		}

		// notified -> get current value from the queue
		std::string log = data_->log_pop();
		// send to clients
		data_->clients_send_all(log);
	}
}

/**
 * @brief Returns the address of the data pointer used by the backend.
 *
 *  This method returns the address of the data pointer created by the backend,
 *  thus it can be used for communication with the connected clients.
 *
 * @return Data*
 */
std::shared_ptr<Data>
Backend::get_data()
{
	return data_;
}

} // namespace rcll::websocket
