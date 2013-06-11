
/***************************************************************************
 *  refbox.h - LLSF RefBox main program
 *
 *  Created: Thu Feb 07 11:02:51 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LLSF_REFBOX_REFBOX_H_
#define __LLSF_REFBOX_REFBOX_H_

#include <boost/asio.hpp>
#include <google/protobuf/message.h>
#include <logging/logger.h>
#include <core/threading/mutex.h>
#include <core/threading/mutex_locker.h>
#include <protobuf_comm/server.h>

#include <clipsmm.h>
#ifdef HAVE_MONGODB
#  include <mongo/bson/bson.h>
#endif


namespace protobuf_clips {
  class ClipsProtobufCommunicator;
}

namespace llsf_sps {
  class SPSComm;
}

#ifdef HAVE_MONGODB
class MongoDBLogProtobuf;
namespace mongo {
  class BSONObjBuilder;
  class DBClientBase;
  class BSONObj;
}
#endif

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class Configuration;
class MultiLogger;

class LLSFRefBox
{
 public:
  LLSFRefBox(int argc, char **argv);
  ~LLSFRefBox();

  int run();

  void handle_signal(const boost::system::error_code& error, int signum);

 private: // methods
  void start_timer();
  void handle_timer(const boost::system::error_code& error);

  void setup_protobuf_comm();

  void          start_clips();
  void          setup_clips();
  void          handle_clips_periodic();
  void          setup_clips_mongodb();

  CLIPS::Values clips_now();
  CLIPS::Values clips_get_clips_dirs();
  void          clips_load_config(std::string cfg_prefix);

#ifdef HAVE_MONGODB
  CLIPS::Value  clips_bson_create();
  CLIPS::Value  clips_bson_parse(std::string document);
  void          clips_bson_destroy(void *bson);
  void          clips_bson_append(void *bson, std::string field_name, CLIPS::Value value);
  void          clips_bson_append_array(void *bson,
					std::string field_name, CLIPS::Values values);
  void          clips_bson_append_time(void *bson,
				       std::string field_name, CLIPS::Values time);
  CLIPS::Value  clips_bson_array_start(void *bson, std::string field_name);
  void          clips_bson_array_finish(void *barr);
  void          clips_bson_array_append(void *barr, CLIPS::Value value);
  std::string   clips_bson_tostring(void *bson);
  void          clips_mongodb_upsert(std::string collection, void *bson, CLIPS::Value query);
  void          clips_mongodb_update(std::string collection, void *bson, CLIPS::Value query);
  void          clips_mongodb_replace(std::string collection, void *bson, CLIPS::Value query);
  void          clips_mongodb_insert(std::string collection, void *bson);
  void          mongodb_update(std::string &collection, mongo::BSONObj obj,
			       CLIPS::Value &query, bool upsert);
#endif

  void          clips_sps_set_signal(std::string machine, std::string light, std::string state);
  void          sps_read_rfids();

  void handle_server_client_msg(protobuf_comm::ProtobufStreamServer::ClientID client,
				uint16_t component_id, uint16_t msg_type,
				std::shared_ptr<google::protobuf::Message> msg);
  void handle_server_client_fail(protobuf_comm::ProtobufStreamServer::ClientID client,
				 uint16_t component_id, uint16_t msg_type,
				 std::string msg);
  void handle_peer_msg(boost::asio::ip::udp::endpoint &endpoint,
		       uint16_t component_id, uint16_t msg_type,
		       std::shared_ptr<google::protobuf::Message> msg);

  void handle_server_sent_msg(protobuf_comm::ProtobufStreamServer::ClientID client,
			      std::shared_ptr<google::protobuf::Message> msg);

  void handle_peer_sent_msg(std::shared_ptr<google::protobuf::Message> msg);

  void handle_client_sent_msg(std::string host, unsigned short port,
			      std::shared_ptr<google::protobuf::Message> msg);

#ifdef HAVE_MONGODB
  void add_comp_type(google::protobuf::Message &m, mongo::BSONObjBuilder *b);
#endif

 private: // members
  Configuration *config_;
  Logger        *logger_;
  MultiLogger   *clips_logger_;
  Logger::LogLevel log_level_;
  llsf_sps::SPSComm *sps_;
  protobuf_clips::ClipsProtobufCommunicator *pb_comm_;

  CLIPS::Environment                       *clips_;
  //std::recursive_mutex                      clips_mutex_;
  fawkes::Mutex                             clips_mutex_;
  std::map<long int, CLIPS::Fact::pointer>  clips_msg_facts_;

  boost::asio::io_service      io_service_;
  boost::asio::deadline_timer  timer_;
  boost::posix_time::ptime     timer_last_;

  unsigned int cfg_timer_interval_;
  std::string  cfg_clips_dir_;

#ifdef HAVE_MONGODB
  bool                cfg_mongodb_enabled_;
  std::string         cfg_mongodb_hostport_;
  std::string         cfg_mongodb_clips_coll_;
  MongoDBLogProtobuf  *mongodb_protobuf_;
  mongo::DBClientBase *mongodb_;
#endif
};


} // end of namespace llsfrb

#endif
