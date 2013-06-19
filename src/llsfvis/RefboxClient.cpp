/*
 * RefboxClient.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "RefboxClient.h"
#include <msgs/GameState.pb.h>
#include <msgs/OrderInfo.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/MachineInfo.pb.h>
#include <msgs/AttentionMessage.pb.h>
#include <msgs/VersionInfo.pb.h>
#include <msgs/MachineCommands.pb.h>
#include <logging/llsf_log_msgs/LogMessage.pb.h>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <config/yaml.h>

namespace LLSFVis {

using namespace protobuf_comm;

RefboxClient::RefboxClient(MainWindow& mainWindow) :
		mainWindow_(mainWindow) {
	connected_ = false;
	dispatcher_.connect(sigc::mem_fun(*this, &RefboxClient::process_queue));

	llsfrb::Configuration *config = new llsfrb::YamlConfiguration(CONFDIR);
	config->load("config.yaml");

	if (config->exists("/llsfrb/shell/refbox-host")
			&& config->exists("/llsfrb/shell/refbox-port")) {
		refbox_host_ = config->get_string("/llsfrb/shell/refbox-host");
		refbox_port_ = config->get_uint("/llsfrb/shell/refbox-port");
	} else {
		refbox_host_ = "localhost";
		refbox_port_ = 4444;
	}
	client = new ProtobufStreamClient();
	MessageRegister & message_register = client->message_register();
	message_register.add_message_type<llsf_msgs::GameState>();
	message_register.add_message_type<llsf_msgs::RobotInfo>();
	message_register.add_message_type<llsf_msgs::MachineInfo>();
	message_register.add_message_type<llsf_msgs::AttentionMessage>();
	message_register.add_message_type<llsf_msgs::PuckInfo>();
	message_register.add_message_type<llsf_msgs::OrderInfo>();
	message_register.add_message_type<llsf_msgs::VersionInfo>();
	message_register.add_message_type<llsf_log_msgs::LogMessage>();
	establish_connection();
	client->signal_connected().connect(
			boost::bind(&RefboxClient::client_connected, this));
	client->signal_disconnected().connect(
			boost::bind(&RefboxClient::client_disconnected, this,
					boost::asio::placeholders::error));
	client->signal_received().connect(
			boost::bind(&RefboxClient::client_msg, this, _1, _2, _3));

	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(
			boost::bind(&RefboxClient::handle_signal, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::signal_number));
}

void RefboxClient::establish_connection() {

	printf("trying to (re-)connect to refbox\n");
	client->async_connect(refbox_host_.c_str(), refbox_port_);

}

RefboxClient::~RefboxClient() {
	// TODO Auto-generated destructor stub
}

void RefboxClient::client_connected() {
	printf("connected!");
	connected_ = true;
	mainWindow_.add_log_message("Refbox connected");
}

void RefboxClient::client_disconnected(const boost::system::error_code &error) {
	printf("disconnected!");
	if (connected_){
	mainWindow_.add_log_message("Refbox disconnected, waiting for reconnection..");
	connected_ = false;
	}
	//if (connected_ == true) {
	//	connected_ = false;

	sleep(3);
	establish_connection();
}

void RefboxClient::client_msg(uint16_t comp_id, uint16_t msg_type,
		std::shared_ptr<google::protobuf::Message> msg) {
	std::lock_guard<std::mutex> lock(mutex_);
	msg_queue_.push(msg);
	dispatcher_();
}

void RefboxClient::process_queue() {
	std::shared_ptr<google::protobuf::Message> msg;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		msg = msg_queue_.front();
		msg_queue_.pop();
	}

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

	std::shared_ptr<llsf_msgs::MachineInfo> mspecs;
	if ((mspecs = std::dynamic_pointer_cast < llsf_msgs::MachineInfo > (msg))) {
		mainWindow_.update_machines(*mspecs);
		return;
	}

	std::shared_ptr<llsf_msgs::AttentionMessage> am;
	if ((am = std::dynamic_pointer_cast < llsf_msgs::AttentionMessage > (msg))) {
		mainWindow_.set_attention_msg(*am);
		return;
	}

	std::shared_ptr<llsf_msgs::PuckInfo> puck;
	if ((puck = std::dynamic_pointer_cast < llsf_msgs::PuckInfo > (msg))) {
		mainWindow_.update_pucks(*puck);
		return;
	}

	std::shared_ptr<llsf_msgs::OrderInfo> order;
	if ((order = std::dynamic_pointer_cast < llsf_msgs::OrderInfo > (msg))) {
		mainWindow_.update_orders(*order);
		return;
	}
}

void RefboxClient::on_signal_send_msg(google::protobuf::Message &m) {
	client->send(m);
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
