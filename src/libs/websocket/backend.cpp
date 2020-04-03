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
 *  Construct a new Backend object with assigned data and server.
 * 
 * @param data_ data object to be used by the backend
 */
Backend::Backend()
{
    server_ = Server(&data_);
}

/**
 * @brief Launches (web-)socket server thread and backend thread.
 * 
 * @param port tcp port of the websocket server
 * @param ws_mode true if websocket only mode is activated
 */
void Backend::start(uint port, bool ws_mode) {
    //configure server
    server_.configure(port, ws_mode);
    // launch server thread
    server_t_ = std::thread(&Server::operator(), server_);
    // launch backend thread
    backend_t_ = std::thread(&Backend::operator(), this);
}

/**
 * @brief Operator runs the backend in the current thread
 * 
 *  This operator runs the webfrontend backend; works 
 *  through the message queue. 
 * 
 */
void Backend::operator()()
{
    // message queue handler -> consumer
    //std::cout << "Starting Message Thread" << std::endl;
    bool msgs_running = true;
    while (msgs_running)
    {
        // block until new message available
        data_.log_wait();

        // notified -> get current value from the queue
        std::string log = data_.log_pop();
        // send to clients
        //std::cout << log << std::endl;
        data_.clients_send_all(log);
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
Data* Backend::get_data() {
    return &(data_);
}


} // namespace llsfrb::websocket