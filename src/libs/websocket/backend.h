/***************************************************************************
 *  backend.h - class to provide backend functionality for webfrontend
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

#ifndef _PLUGINS_WEBSOCKET_BACKEND_H_
#define _PLUGINS_WEBSOCKET_BACKEND_H_

#include "data.h"
#include "logging/logger.h"
#include "server.h"

namespace llsfrb::websocket {

class Backend
{
public:
	Backend(Logger *logger_);

	void  operator()();
	void  start(uint port, bool ws_mode = true, bool allow_control_all = false);
	Data *get_data();

private:
	Logger *    logger_;
	Data *      data_;
	Server      server_;
	std::thread backend_t_;
	std::thread server_t_;
};

} // namespace llsfrb::websocket

#endif