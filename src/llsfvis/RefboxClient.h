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
#include "MainWindow.h"
#include "msgs/PuckInfo.pb.h"

namespace LLSFVis {

class RefboxClient : public sigc::trackable {
public:
	RefboxClient(MainWindow& mainWindow);
	virtual ~RefboxClient();
	void on_signal_remove_puck(llsf_msgs::RemovePuckFromMachine& puck);
private:
	MainWindow& mainWindow_;
	protobuf_comm::ProtobufStreamClient *client;
	boost::asio::io_service io_service_;
	void client_connected();
	void client_disconnected(const boost::system::error_code &error);
	void client_msg(uint16_t comp_id, uint16_t msg_type,
			std::shared_ptr<google::protobuf::Message> msg);
	void handle_signal(const boost::system::error_code& error, int signum);


};

} /* namespace LLSFVis */
#endif /* REFBOXCLIENT_H_ */
