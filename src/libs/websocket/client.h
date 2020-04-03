/***************************************************************************
 *  client.h - class to manage connected frontend clients
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

#ifndef _PLUGINS_WEBSOCKET_CLIENT_H_
#define _PLUGINS_WEBSOCKET_CLIENT_H_

#include <iostream>
#include <sys/socket.h>
#include <mutex>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

#include "logging/logger.h"

namespace llsfrb::websocket
{
class Client
{
public:
  virtual bool send(std::string msg) = 0;
  virtual std::string read() = 0;

protected:
  std::mutex rd_mu;
  std::mutex wr_mu;
};

class ClientWS : public Client
{
public:
  ClientWS(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket, Logger *logger_);

  bool send(std::string msg);
  std::string read();

private:
  std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;
  Logger *logger_;
};

class ClientS : public Client
{
public:
  ClientS(std::shared_ptr<boost::asio::ip::tcp::socket> socket, Logger *logger_);

  bool send(std::string msg);
  std::string read();

private:
  std::shared_ptr<boost::asio::ip::tcp::socket> socket;
  Logger *logger_;
};
} // namespace llsfrb::websocket
#endif