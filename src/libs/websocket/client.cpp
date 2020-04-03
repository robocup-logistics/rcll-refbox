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

#include <iostream>
#include <sys/socket.h>
#include <mutex>
#include <string>
#include <chrono>
#include <condition_variable>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

namespace llsfrb::websocket
{

/**
 * @brief Construct a new ClientWS::ClientWS object
 * 
 * @param socket Established WebSocket socket shared pointer user for this client
 */
ClientWS::ClientWS(std::shared_ptr<boost::beast::websocket::stream<tcp::socket>> socket, Logger *logger_)
    : socket(socket), logger_(logger_)
{
    socket->accept();
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
bool ClientWS::send(std::string msg)
{
    const std::lock_guard<std::mutex> lock(wr_mu);

    boost::system::error_code error;
    socket->write(boost::asio::buffer(msg + "\n"), error);
    if (error && error != boost::asio::error::eof)
    {
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
std::string ClientWS::read()
{
    const std::lock_guard<std::mutex> lock(rd_mu);

    boost::asio::streambuf buf;
    socket->read(buf);
    std::string data = boost::asio::buffer_cast<const char *>(buf.data());

    return data;
}

/**
 * @brief Construct a new ClientS::ClientS object
 * 
 * @param socket TCP socket over which client communication happens
 * @param logger_ Logger instance to be used 
 */
ClientS::ClientS(std::shared_ptr<tcp::socket> socket, Logger *logger_) : socket(socket), logger_(logger_) {}

/**
 * @brief Send string message to client 
 * 
 *  Thread-safe TCP socket send function, sends a given string message with a trailing newline to the client. 
 * 
 * @param msg message to be sent
 * @return true send was successful
 * @return false send was unsuccessful
 */
bool ClientS::send(std::string msg)
{
    const std::lock_guard<std::mutex> lock(wr_mu);

    boost::system::error_code error;
    boost::asio::write(*socket, boost::asio::buffer(msg + "\n"), error);
    if (error && error != boost::asio::error::eof)
    {
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
std::string ClientS::read()
{
    const std::lock_guard<std::mutex> lock(rd_mu);

    boost::asio::streambuf buf;
    boost::asio::read_until(*socket, buf, "\n");
    std::string data = boost::asio::buffer_cast<const char *>(buf.data());

    return data;
}
} // namespace llsfrb::websocket