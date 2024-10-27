/***************************************************************************
 *  rebroadcaster.h - class to rebroadcast protobuf messages
 *
 *  It is ported from the gazsim-comm plugin of fawkes
 *
 *  Created: 27. Oct 2024
 *  Copyright  2024       Tarik Viehmann  [viehmann@kbsg.rwth-aachen.de]
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

#ifndef _PLUGINS_GAZSIM_COMM_COMM_THREAD_H_
#define _PLUGINS_GAZSIM_COMM_COMM_THREAD_H_

#include <config/config.h>
#include <config/yaml.h>
#include <google/protobuf/message.h>
#include <logging/logger.h>
#include <logging/multi.h>
#include <protobuf_comm/message_register.h>
#include <protobuf_comm/peer.h>

#include <boost/asio.hpp>
#include <list>

namespace protobuf_comm {
class ProtobufStreamClient;
}
namespace rcll {
class Configuration;
class MultiLogger;
class ProtoRebroadcaster
{
public:
	ProtoRebroadcaster(std::shared_ptr<Configuration> config);

private:
	void peer_send_error(std::string address, unsigned int port, std::string err);
	void receive_raw_msg(boost::asio::ip::udp::endpoint &endpoint,
	                     protobuf_comm::frame_header_t  &header,
	                     void                           *data,
	                     size_t                          length);

private:
	std::vector<protobuf_comm::ProtobufBroadcastPeer *> peers_;
	std::vector<protobuf_comm::ProtobufBroadcastPeer *> peers_crypto1_;
	std::vector<protobuf_comm::ProtobufBroadcastPeer *> peers_crypto2_;

	//config values
	std::vector<std::string>  addresses_;
	std::vector<unsigned int> send_ports_;
	std::vector<unsigned int> recv_ports_;
	std::vector<unsigned int> send_ports_crypto1_;
	std::vector<unsigned int> recv_ports_crypto1_;
	std::vector<unsigned int> send_ports_crypto2_;
	std::vector<unsigned int> recv_ports_crypto2_;

	bool use_crypto1_, use_crypto2_;

	std::vector<std::string> proto_dirs_;
	double                   package_loss_;

	std::shared_ptr<Configuration> config_;
	std::unique_ptr<MultiLogger>   logger_;

	Logger::LogLevel log_level_;
};
} // namespace rcll

#endif
