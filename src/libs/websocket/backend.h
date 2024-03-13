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

#include <clipsmm.h>

using namespace fawkes;
namespace llsfrb::websocket {

class Backend
{
public:
	Backend(Logger                             *logger,
	        std::shared_ptr<CLIPS::Environment> env,
	        fawkes::Mutex                      &env_mutex,
	        boost::asio::io_service            &io_service,
	        uint                                port,
	        bool                                ws_mode           = true,
	        bool                                allow_control_all = false);

	~Backend();

	void                  operator()();
	std::shared_ptr<Data> get_data();

private:
	std::shared_ptr<Logger> logger_;
	std::shared_ptr<Data>   data_;
	Server                  server_;
	std::thread             backend_t_;
	std::thread             server_t_;
	bool                    shutdown_ = false;
};

} // namespace llsfrb::websocket

#endif
