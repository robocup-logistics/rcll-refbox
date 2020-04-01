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

#include <iostream>
#include <string>
#include <thread>

#include "server.h"
#include "backend.h"

namespace llsfrb::websocket
{

/**
 * @brief Construct a new Backend::Backend object
 * 
 *  Construct a new Backend object, the () operator of which can be used to launch the backend. 
 * 
 * @param data_ data object to be used by the backend
 */
Backend::Backend(Data *data_) : data_(data_), server_(Server(data_))
{
}

/**
 * @brief Operator runs the backend in the current thread
 * 
 *  This operator runs the webfrontend backend, launches the webserver and works 
 *  through the message queue. 
 * 
 */
void Backend::operator()() const
{
    // launch server thread
    std::thread server_t(server_);

    // message queue handler -> consumer
    std::cout << "Starting Message Thread" << std::endl;
    bool msgs_running = true;
    while (msgs_running)
    {
        // block until new message available
        data_->log_wait();

        // notified -> get current value from the queue
        std::string log = data_->log_pop();
        // send to clients
        //std::cout << log << std::endl;
        data_->clients_send_all(log);
    }
}

} // namespace llsfrb::websocket