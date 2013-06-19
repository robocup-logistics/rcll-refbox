/*
 * RefboxClient.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef REFBOXCLIENT_H_
#define REFBOXCLIENT_H_

#include <protobuf_comm/client.h>
#include <boost/asio.hpp>
#include <sigc++/sigc++.h>
#include <queue>
#include <mutex>
#include "MainWindow.h"
#include "msgs/PuckInfo.pb.h"

namespace LLSFVis {

class RefboxClient : public sigc::trackable {
public:
	RefboxClient(MainWindow& mainWindow);
	virtual ~RefboxClient();
	void on_signal_send_msg(google::protobuf::Message &m);
	void process_queue();
private:
	void establish_connection();


private:
	MainWindow& mainWindow_;
	protobuf_comm::ProtobufStreamClient *client;
	boost::asio::io_service io_service_;
	void client_connected();
	void client_disconnected(const boost::system::error_code &error);
	void client_msg(uint16_t comp_id, uint16_t msg_type,
			std::shared_ptr<google::protobuf::Message> msg);
	void handle_signal(const boost::system::error_code& error, int signum);
	std::queue<std::shared_ptr<google::protobuf::Message> > msg_queue_;
	std::mutex mutex_;
	Glib::Dispatcher dispatcher_;

	bool connected_;
	std::string refbox_host_;
	int refbox_port_;


};

} /* namespace LLSFVis */
#endif /* REFBOXCLIENT_H_ */
