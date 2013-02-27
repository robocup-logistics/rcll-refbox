/*
 * RefboxClient.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "RefboxClient.h"
#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/MachineSpec.pb.h>
#include <msgs/AttentionMessage.pb.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

namespace LLSFVis {

using namespace protobuf_comm;

RefboxClient::RefboxClient(MainWindow& mainWindow) :
		mainWindow_(mainWindow) {
	client = new ProtobufStreamClient();
	MessageRegister & message_register = client->message_register();
	message_register.add_message_type<llsf_msgs::GameState>();
	message_register.add_message_type<llsf_msgs::RobotInfo>();
	message_register.add_message_type<llsf_msgs::MachineSpecs>();
	message_register.add_message_type<llsf_msgs::AttentionMessage>();

	client->signal_connected().connect(
			boost::bind(&RefboxClient::client_connected, this));
	client->signal_disconnected().connect(
			boost::bind(&RefboxClient::client_disconnected, this,
					boost::asio::placeholders::error));
	client->signal_received().connect(
			boost::bind(&RefboxClient::client_msg, this, _1, _2, _3));

	client->async_connect("localhost", 4444);

	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(
			boost::bind(&RefboxClient::handle_signal, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::signal_number));

}

RefboxClient::~RefboxClient() {
	// TODO Auto-generated destructor stub
}

void RefboxClient::client_connected() {
	mainWindow_.add_log_message("Refbox connected");
}

void RefboxClient::client_disconnected(const boost::system::error_code &error) {
	mainWindow_.add_log_message("Refbox disconnected: " + error.message());
}

void RefboxClient::client_msg(uint16_t comp_id, uint16_t msg_type,
		std::shared_ptr<google::protobuf::Message> msg) {
	std::shared_ptr<llsf_msgs::GameState> g;
	if ((g = std::dynamic_pointer_cast < llsf_msgs::GameState > (msg))) {
		mainWindow_.update_game_state(*g);
		return;
	}

	std::shared_ptr<llsf_msgs::RobotInfo> r;
	if ((r = std::dynamic_pointer_cast < llsf_msgs::RobotInfo > (msg))) {
		mainWindow_.update_robots(*r);
		return;

	}

	std::shared_ptr<llsf_msgs::MachineSpecs> mspecs;
	if ((mspecs = std::dynamic_pointer_cast < llsf_msgs::MachineSpecs > (msg))) {
		mainWindow_.update_machines(*mspecs);
		return;
	}

	std::shared_ptr<llsf_msgs::AttentionMessage> am;
	if ((am = std::dynamic_pointer_cast < llsf_msgs::AttentionMessage > (msg))) {
		mainWindow_.set_attention_msg(*am);
		return;
	}

}

/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void RefboxClient::handle_signal(const boost::system::error_code& error,
		int signum) {
	io_service_.stop();
}

} /* namespace LLSFVis */
