/***************************************************************************
 *  server.h - class to provide server functionality to handle incoming connections
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

#ifndef _PLUGINS_WEBSOCKET_SERVER_H_
#define _PLUGINS_WEBSOCKET_SERVER_H_

#include "data.h"
#include "logging/logger.h"

#include <boost/asio.hpp>

namespace rcll::websocket {

class Server
{
public:
	Server(std::shared_ptr<Data>    data,
	       std::shared_ptr<Logger>  logger,
	       boost::asio::io_service &io_service,
	       uint                     port,
	       bool                     ws_mode,
	       bool                     allow_control_all);

	void shutdown();

	void operator()();

private:
	void handle_accept(const boost::system::error_code &error, boost::asio::ip::tcp::socket &socket);
	void do_accept();

	std::shared_ptr<Data>          data_;
	std::shared_ptr<Logger>        logger_;
	uint                           port_ = 1234;
	boost::asio::ip::tcp::socket   socket_;
	boost::asio::ip::tcp::acceptor acceptor_;
	bool                           ws_mode_           = true;
	bool                           allow_control_all_ = false;
	bool                           shutdown_          = false;
};

} // namespace rcll::websocket

#endif
