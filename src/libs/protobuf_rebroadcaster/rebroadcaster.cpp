// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  rebroadcaster.cpp - class to rebroadcast protobuf messages
 *
 *  It is ported from the gazsim-comm plugin of fawkes
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
#include "rebroadcaster.h"

#include <logging/console.h>
#include <logging/multi.h>
#include <protobuf_comm/message_register.h>
#include <protobuf_comm/peer.h>

#include <algorithm>
#include <stdlib.h>

using namespace protobuf_comm;
using namespace boost::placeholders;

namespace rcll {
ProtoRebroadcaster::ProtoRebroadcaster(std::shared_ptr<Configuration> config) : config_(config)
{
	log_level_ = Logger::LL_INFO;
	try {
		std::string ll = config_->get_string("/llsfrb/log/level");
		if (ll == "debug") {
			log_level_ = Logger::LL_DEBUG;
		} else if (ll == "info") {
			log_level_ = Logger::LL_INFO;
		} else if (ll == "warn") {
			log_level_ = Logger::LL_WARN;
		} else if (ll == "error") {
			log_level_ = Logger::LL_ERROR;
		}
	} catch (fawkes::Exception &e) {
	} // ignored, use default
	logger_ = std::make_unique<MultiLogger>();
	logger_->add_logger(new ConsoleLogger(log_level_));

	//read config values
	proto_dirs_         = config_->get_strings("/llsfrb/comm/protobuf-dirs");
	package_loss_       = config_->get_float("/llsfrb/comm/rebroadcaster/package-loss");
	addresses_          = config_->get_strings("/llsfrb/comm/rebroadcaster/addresses");
	send_ports_         = config_->get_uints("/llsfrb/comm/rebroadcaster/send-ports");
	recv_ports_         = config_->get_uints("/llsfrb/comm/rebroadcaster/recv-ports");
	use_crypto1_        = config_->get_bool("/llsfrb/comm/rebroadcaster/use-crypto1");
	use_crypto2_        = config_->get_bool("/llsfrb/comm/rebroadcaster/use-crypto1");
	send_ports_crypto1_ = config_->get_uints("/llsfrb/comm/rebroadcaster/send-ports-crypto1");
	recv_ports_crypto1_ = config_->get_uints("/llsfrb/comm/rebroadcaster/recv-ports-crypto1");
	send_ports_crypto2_ = config_->get_uints("/llsfrb/comm/rebroadcaster/send-ports-crypto2");
	recv_ports_crypto2_ = config_->get_uints("/llsfrb/comm/rebroadcaster/recv-ports-crypto2");
	if (addresses_.size() != send_ports_.size() || addresses_.size() != recv_ports_.size()
	    || (use_crypto1_ && addresses_.size() != send_ports_crypto1_.size())
	    || (use_crypto1_ && addresses_.size() != recv_ports_crypto1_.size())
	    || (use_crypto2_ && addresses_.size() != send_ports_crypto2_.size())
	    || (use_crypto2_ && addresses_.size() != recv_ports_crypto2_.size())) {
		logger_->log_warn("ProtoRebroadcaster",
		                  "/llsfrb/comm/rebroadcaster/ has an invalid configuration!");
	}

	//resolve proto paths
	try {
		for (size_t i = 0; i < proto_dirs_.size(); ++i) {
			std::string::size_type pos;
			if ((pos = proto_dirs_[i].find("@BASEDIR@")) != std::string::npos) {
				proto_dirs_[i].replace(pos, 9, BASEDIR);
			}
			if ((pos = proto_dirs_[i].find("@RESDIR@")) != std::string::npos) {
				proto_dirs_[i].replace(pos, 8, RESDIR);
			}
			if ((pos = proto_dirs_[i].find("@CONFDIR@")) != std::string::npos) {
				proto_dirs_[i].replace(pos, 9, CONFDIR);
			}
			if (proto_dirs_[i][proto_dirs_.size() - 1] != '/') {
				proto_dirs_[i] += "/";
			}
		}
	} catch (fawkes::Exception &e) {
		logger_->log_warn("ProtoRebroadcaster",
		                  "Failed to load proto paths from config, exception follows");
		logger_->log_warn("ProtoRebroadcaster", e);
	}

	//create peer connections
	peers_.resize(addresses_.size());
	peers_crypto1_.resize(addresses_.size());
	peers_crypto2_.resize(addresses_.size());
	for (unsigned int i = 0; i < addresses_.size(); i++) {
		peers_[i] =
		  new ProtobufBroadcastPeer(addresses_[i], send_ports_[i], recv_ports_[i], proto_dirs_);
		peers_[i]->signal_received_raw().connect(
		  boost::bind(&ProtoRebroadcaster::receive_raw_msg, this, _1, _2, _3, _4));
		peers_[i]->signal_send_error().connect(
		  boost::bind(&ProtoRebroadcaster::peer_send_error, this, addresses_[i], send_ports_[i], _1));
		if (use_crypto1_) {
			peers_crypto1_[i] = new ProtobufBroadcastPeer(addresses_[i],
			                                              send_ports_crypto1_[i],
			                                              recv_ports_crypto1_[i],
			                                              proto_dirs_);
			peers_crypto1_[i]->signal_received_raw().connect(
			  boost::bind(&ProtoRebroadcaster::receive_raw_msg, this, _1, _2, _3, _4));
			peers_crypto1_[i]->signal_send_error().connect(boost::bind(
			  &ProtoRebroadcaster::peer_send_error, this, addresses_[i], send_ports_crypto1_[i], _1));
		}
		if (use_crypto2_) {
			peers_crypto2_[i] = new ProtobufBroadcastPeer(addresses_[i],
			                                              send_ports_crypto2_[i],
			                                              recv_ports_crypto2_[i],
			                                              proto_dirs_);
			peers_crypto2_[i]->signal_received_raw().connect(
			  boost::bind(&ProtoRebroadcaster::receive_raw_msg, this, _1, _2, _3, _4));
			peers_crypto2_[i]->signal_send_error().connect(boost::bind(
			  &ProtoRebroadcaster::peer_send_error, this, addresses_[i], send_ports_crypto2_[i], _1));
		}
	}
}

/**
 * Receive and forward raw msg
 * @param endpoint port msg received from
 * @param header header of the msg
 * @param data data stream
 * @param length length of the data stream
 */
void
ProtoRebroadcaster::receive_raw_msg(boost::asio::ip::udp::endpoint &endpoint,
                                    protobuf_comm::frame_header_t  &header,
                                    void                           *data,
                                    size_t                          length)
{
	//logger_->log_info(name(), "Got raw Message from port %d", endpoint.port());
	unsigned int incoming_peer_port = endpoint.port(); //this is suprisingly the send-port

	//simulate package loss
	double rnd = ((double)rand()) / ((double)RAND_MAX); //0.0 <= rnd <= 1.0
	if (rnd < package_loss_) {
		return;
	}

	//check which set of peers the message comes from
	std::vector<protobuf_comm::ProtobufBroadcastPeer *> peers;
	std::vector<unsigned int>                           send_ports;
	if (std::find(send_ports_.begin(), send_ports_.end(), incoming_peer_port) != send_ports_.end()) {
		peers      = peers_;
		send_ports = send_ports_;
	} else if (use_crypto1_
	           && std::find(send_ports_crypto1_.begin(),
	                        send_ports_crypto1_.end(),
	                        incoming_peer_port)
	                != send_ports_crypto1_.end()) {
		peers      = peers_crypto1_;
		send_ports = send_ports_crypto1_;
	} else if (use_crypto2_
	           && std::find(send_ports_crypto2_.begin(),
	                        send_ports_crypto2_.end(),
	                        incoming_peer_port)
	                != send_ports_crypto2_.end()) {
		peers      = peers_crypto2_;
		send_ports = send_ports_crypto2_;
	}

	//send message to all other peers
	for (unsigned int i = 0; i < peers.size(); i++) {
		if (send_ports[i] != incoming_peer_port) {
			peers[i]->send_raw(header, data, length);
		}
	}
}

void
ProtoRebroadcaster::peer_send_error(std::string address, unsigned int port, std::string err)
{
	logger_->log_warn(
	  "ProtoRebroadcaster", "Peer send error for %s:%u: %s", address.c_str(), port, err.c_str());
}

} // namespace rcll
