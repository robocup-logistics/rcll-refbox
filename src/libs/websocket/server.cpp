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

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <sys/socket.h>

#include "server.h"
#include "client.h"
#include "data.h"

using boost::asio::ip::tcp;

namespace llsfrb::websocket
{

/**
 * @brief Construct a new Server::Server object
 * 
 *  Constructs a new Server object and assigns the used backend data pointer.
 * 
 * @param data_ptr pointer to Data object that is used for this session
 * @param logger_ logger used by the backend
 */
Server::Server(Data *data_, Logger *logger_) : data_(data_), logger_(logger_)
{
}

Server::Server()
{
}

/**
 * @brief Runs the Socket/Websocket Server
 *  This operator runs the socket or websocket server (currently based on WS_MODE predef).
 *  It launches an endless accepting loop, that accepts new connections and creates the 
 *  necessary objects required by the backend to work with them. 
 * 
 */
void Server::operator()()
{
    // listen for new connection
    boost::asio::io_service io_service;
    tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_));

    // acceptor loop
    while (true)
    {
        std::shared_ptr<tcp::socket> socket(new tcp::socket(io_service));
        acceptor_.accept(*socket);

        if (ws_mode_)
        {
            // websocket approach
            std::shared_ptr<boost::beast::websocket::stream<tcp::socket>> web_socket(
                new boost::beast::websocket::stream<tcp::socket>(std::move(*socket)));
            std::shared_ptr<Client> client(
                new ClientWS(web_socket, logger_));
            data_->clients_add(client);
        }
        else
        {
            // socket approach
            std::shared_ptr<Client> client(
                new ClientS(socket, logger_));
            data_->clients_add(client);
        }

        logger_->log_info("Websocket", "new client connected");
    }
}

/**
 * @brief Configure the servers port and mode prior to start. 
 * 
 * @param port port on which the server runs on
 * @param ws_mode true if websocket only mode
 */
void Server::configure(uint port, bool ws_mode = true)
{
    port_ = port;
    ws_mode_ = ws_mode;
}

} // namespace llsfrb::websocket