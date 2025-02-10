// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

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

#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <sys/socket.h>

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>

using boost::asio::ip::tcp;

namespace rcll::websocket {

/**
 * @brief Construct a new ClientWS::ClientWS object
 *
 * @param socket Established WebSocket socket shared pointer user for this client
 */
ClientWS::ClientWS(std::shared_ptr<boost::beast::websocket::stream<tcp::socket>> socket,
                   std::shared_ptr<Logger>                                       logger,
                   std::shared_ptr<Data>                                         data,
                   bool                                                          can_send)
: socket(socket)
{
	logger_   = logger;
	data_     = data;
	can_send_ = can_send;

	socket->accept();
	client_t = std::thread(&Client::receive_thread, this);
	logger_->log_info("Websocket", "client receive thread started");
	on_connect_update();
}

/**
 * @brief Destroy the ClientWS:: ClientWS object
 *
 */
ClientWS::~ClientWS()
{
	disconnect();
	client_t.join();
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
bool
ClientWS::send(std::string msg)
{
	const std::lock_guard<std::mutex> lock(wr_mu);

	// Split the input string into individual messages
	std::istringstream iss(msg);
	std::string        singleMsg;

	while (std::getline(iss, singleMsg, '\n')) {
		// Send each message separately
		boost::system::error_code error;
		socket->write(boost::asio::buffer(singleMsg + "\n"), error);

		if (error && error != boost::asio::error::eof) {
			return false;
		}
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
std::string
ClientWS::read()
{
	const std::lock_guard<std::mutex> lock(rd_mu);

	boost::asio::streambuf buf;
	socket->read(buf);
	std::string data = boost::asio::buffer_cast<const char *>(buf.data());

	return data;
}

/**
 * @brief WebSocket implementation for close
 *
 */
void
ClientWS::close()
{
	socket->next_layer().close();
}

/**
 * @brief Construct a new ClientS::ClientS object
 *
 * @param socket TCP socket over which client communication happens
 * @param logger Logger instance to be used
 * @param data Data instance to be used
 * @param can_send sets if the connected client's incoming commands are processed
 */
ClientS::ClientS(std::shared_ptr<tcp::socket> socket,
                 std::shared_ptr<Logger>      logger,
                 std::shared_ptr<Data>        data,
                 bool                         can_send)
: socket(socket)
{
	logger_   = logger;
	data_     = data;
	can_send_ = can_send;
	client_t  = std::thread(&Client::receive_thread, this);
	logger_->log_info("Websocket", "TCP-socket client receive thread started");
	on_connect_update();
}

/**
 * @brief Destroy the ClientS::ClientS object
 *
 */
ClientS::~ClientS()
{
	disconnect();
	client_t.join();
}

/**
 * @brief Send string message to client
 *
 *  Thread-safe TCP socket send function, sends a given string message with a trailing newline to the client.
 *
 * @param msg message to be sent
 * @return true send was successful
 * @return false send was unsuccessful
 */
bool
ClientS::send(std::string msg)
{
	const std::lock_guard<std::mutex> lock(wr_mu);

	// Split the input string into individual messages
	std::istringstream iss(msg);
	std::string        singleMsg;

	while (std::getline(iss, singleMsg, '\n')) {
		// Send each message separately
		boost::system::error_code error;
		boost::asio::write(*socket, boost::asio::buffer(singleMsg + "\n"), error);

		if (error && error != boost::asio::error::eof) {
			return false;
		}
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
std::string
ClientS::read()
{
	const std::lock_guard<std::mutex> lock(rd_mu);

	boost::asio::streambuf buf;
	boost::asio::read_until(*socket, buf, "\n");
	std::string data = boost::asio::buffer_cast<const char *>(buf.data());

	return data;
}

/**
 * @brief TCP-Socket implementation for close
 *
 */
void
ClientS::close()
{
	socket->close();
}

/**
 * @brief Handles incoming message requests
 *
 */
void
Client::receive_thread()
{
	rapidjson::Document msgs;
	// Check for existence of rcll-prepare-machine tool for websocket prepare
	std::string prepare_machine_loc       = std::string(BINDIR) + "/rcll-prepare-machine";
	bool        prepare_machine_supported = access(prepare_machine_loc.c_str(), X_OK) == 0;
	std::string prepare_machine_command   = std::string(BINDIR) + "/./rcll-prepare-machine ";
	if (!prepare_machine_supported) {
		logger_->log_error(
		  "Websocket",
		  "Could not find rcll-prepare-machine at %s, machine instructions per websocket ar disabled");
	}

	while (active) {
		try {
			std::string input = read();
			msgs.Parse(input.c_str());

			//check incoming message type and call corresponding CLIPS function
			if (!msgs.IsObject()) {
				logger_->log_error("Websocket", "non JSON message received, won't process");
			} else if (!can_send_) {
				logger_->log_error("Websocket", "non localhost client tried to send command");
			} else if (msgs.HasMember("command")) {
				std::string                command = msgs["command"].GetString();
				rapidjson::SchemaValidator validator(*(data_->command_schema_map[command]));
				if (!msgs.Accept(validator)) {
					logger_->log_error("Websocket", "input JSON is invalid!");
				} else {
					if (strcmp(msgs["command"].GetString(), "set_gamestate") == 0) {
						data_->clips_set_gamestate(msgs["state"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_confval") == 0) {
						data_->clips_set_confval(msgs["path"].GetString(), msgs["value"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "reset") == 0) {
						kill(getpid(), SIGUSR1);
					}
					if (strcmp(msgs["command"].GetString(), "set_gamephase") == 0) {
						data_->clips_set_gamephase(msgs["phase"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "randomize_field") == 0) {
						data_->clips_randomize_field();
					}
					if (strcmp(msgs["command"].GetString(), "set_teamname") == 0) {
						data_->clips_set_teamname(msgs["color"].GetString(), msgs["name"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "confirm_delivery") == 0) {
						data_->clips_confirm_delivery(msgs["delivery_id"].GetInt(),
						                              msgs["correctness"].GetBool(),
						                              msgs["order_id"].GetInt(),
						                              msgs["color"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_order_delivered") == 0) {
						data_->clips_set_order_delivered(msgs["color"].GetString(), msgs["order_id"].GetInt());
					}
					if (strcmp(msgs["command"].GetString(), "set_preset") == 0) {
						data_->clips_set_cfg_preset(msgs["category"].GetString(), msgs["preset"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_machine_state") == 0) {
						data_->clips_production_set_machine_state(msgs["mname"].GetString(),
						                                          msgs["state"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_machine_work_status") == 0) {
						data_->clips_production_set_machine_work_status(msgs["name"].GetString(),
						                                                msgs["busy"].GetBool(),
						                                                msgs["ready"].GetBool());
					}
					if (strcmp(msgs["command"].GetString(), "set_machine_pose") == 0) {
						data_->clips_set_machine_pose(msgs["name"].GetString(),
						                              msgs["rotation"].GetInt(),
						                              msgs["zone"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "machine_add_base") == 0) {
						data_->clips_production_machine_add_base(msgs["mname"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "set_robot_maintenance") == 0) {
						data_->clips_robot_set_robot_maintenance(msgs["robot_number"].GetInt(),
						                                         msgs["team_color"].GetString(),
						                                         msgs["maintenance"].GetBool());
					}
					if (strcmp(msgs["command"].GetString(), "reset_machine_by_team") == 0) {
						data_->clips_production_reset_machine_by_team(msgs["machine_name"].GetString(),
						                                              msgs["team_color"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "add_points_team") == 0) {
						data_->clips_add_points_team(msgs["points"].GetInt(),
						                             msgs["team_color"].GetString(),
						                             msgs["game_time"].GetFloat(),
						                             msgs["phase"].GetString(),
						                             msgs["reason"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "break_machine") == 0) {
						data_->clips_reset_machine(msgs["machine"].GetString());
					}
					if (strcmp(msgs["command"].GetString(), "instruct_bs") == 0) {
						if (prepare_machine_supported) {
							std::ostringstream command;
							command << prepare_machine_command << msgs["team_name"].GetString() << " "
							        << msgs["machine"].GetString() << " " << msgs["side"].GetString() << " "
							        << msgs["base_color"].GetString();
							int result = std::system(command.str().c_str());
							// Check the result
							if (result != 0) {
								logger_->log_error("Websocket",
								                   "Command %s failed with code %i",
								                   command.str().c_str(),
								                   result);
							}
						} else {
							logger_->log_warn("Websocket", "Machine instructions per websocket ar disabled");
						}
					}
					if (strcmp(msgs["command"].GetString(), "instruct_rs") == 0) {
						if (prepare_machine_supported) {
							std::ostringstream command;
							command << prepare_machine_command << msgs["team_name"].GetString() << " "
							        << msgs["machine"].GetString() << " " << msgs["ring_color"].GetString();
							int result = std::system(command.str().c_str());
							// Check the result
							if (result != 0) {
								logger_->log_error("Websocket",
								                   "Command %s failed with code %i",
								                   command.str().c_str(),
								                   result);
							}
						} else {
							logger_->log_warn("Websocket", "Machine instructions per websocket ar disabled");
						}
					}
					if (strcmp(msgs["command"].GetString(), "instruct_cs") == 0) {
						if (prepare_machine_supported) {
							std::ostringstream command;
							command << prepare_machine_command << msgs["team_name"].GetString() << " "
							        << msgs["machine"].GetString() << " " << msgs["operation"].GetString();
							int result = std::system(command.str().c_str());
							// Check the result
							if (result != 0) {
								logger_->log_error("Websocket",
								                   "Command %s failed with code %i",
								                   command.str().c_str(),
								                   result);
							}
						} else {
							logger_->log_warn("Websocket", "Machine instructions per websocket ar disabled");
						}
					}
					if (strcmp(msgs["command"].GetString(), "instruct_ds") == 0) {
						if (prepare_machine_supported) {
							std::ostringstream command;
							command << prepare_machine_command << msgs["team_name"].GetString() << " "
							        << msgs["machine"].GetString() << " " << msgs["order"].GetInt();
							int result = std::system(command.str().c_str());
							// Check the result
							if (result != 0) {
								logger_->log_error("Websocket",
								                   "Command %s failed with code %i",
								                   command.str().c_str(),
								                   result);
							}
						} else {
							logger_->log_warn("Websocket", "Machine instructions per websocket ar disabled");
						}
					}
					if (strcmp(msgs["command"].GetString(), "instruct_ss") == 0) {
						if (prepare_machine_supported) {
							std::ostringstream command;
							command << prepare_machine_command << msgs["team_name"].GetString() << " "
							        << msgs["machine"].GetString() << " " << msgs["operation"].GetString() << " "
							        << msgs["shelf"].GetInt() << " " << msgs["slot"].GetInt();
							int result = std::system(command.str().c_str());
							// Check the result
							if (result != 0) {
								logger_->log_error("Websocket",
								                   "Command %s failed with code %i",
								                   command.str().c_str(),
								                   result);
							}
						} else {
							logger_->log_warn("Websocket", "Machine instructions per websocket ar disabled");
						}
					}
					logger_->log_debug("Websocket", "got %s", msgs["command"].GetString());
				}
			} else {
				logger_->log_error("Websocket", "malformed message received, won't be processed");
			}

		} catch (std::exception &e) {
			logger_->log_debug("Websocket", "caught exception while receiving. %s", e.what());
			disconnect();
		} catch (...) {
			logger_->log_debug("Websocket", "caught unknown exception while receiving");
			disconnect();
		}
	}
}

/**
 * @brief Disconnects client by closing connection and stopping receive thread
 *
 */
void
Client::disconnect()
{
	if (active) {
		active = false;
	}
	close();
	logger_->log_info("Websocket", "client disconnected");
}

/**
 * @brief Send the current fact base to a freshly connected client
 *
 */
void
Client::on_connect_update()
{
	logger_->log_info("Websocket", "send on connect update");
	std::string gamephase = data_->get_gamephase();
	std::string gamestate = data_->get_gamestate();

	send(data_->on_connect_known_teams());
	send(data_->on_connect_game_state());
	send(data_->on_connect_config());
	send(data_->on_connect_cfg_preset());
	send(data_->on_connect_agent_task_info());

	if (gamestate == "RUNNING" || gamestate == "PAUSED") {
		send(data_->on_connect_machine_info());
		send(data_->on_connect_robot_info());
		send(data_->on_connect_workpiece_info());
		send(data_->on_connect_points());
	}
	if (gamephase == "PRODUCTION" || gamephase == "POST_GAME") {
		send(data_->on_connect_order_info());
		send(data_->on_connect_ring_spec());
	}
	if (gamephase == "SETUP" || gamephase == "EXPLORATION") {
		send(data_->on_connect_ring_spec());
	}
}
} // namespace rcll::websocket
