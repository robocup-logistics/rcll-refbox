
/***************************************************************************
 *  refbox.cpp - LLSF RefBox main program
 *
 *  Created: Thu Feb 07 11:04:17 2013
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

#include "refbox.h"
#include "clips_logger.h"

#include <core/threading/mutex.h>
#include <core/version.h>
#include <config/yaml.h>
#include <protobuf_clips/communicator.h>
#include <protobuf_comm/peer.h>
#include <mps_placing_clips/mps_placing_clips.h>
#include <llsf_sps/sps_comm.h>
#include <logging/multi.h>
#include <logging/file.h>
#include <logging/network.h>
#include <logging/console.h>
#include <llsf_sps/mps_refbox_interface.h>
#include <llsf_sps/mps_incoming_station.h>
#include <llsf_sps/mps_pick_place_1.h>
#include <llsf_sps/mps_pick_place_2.h>
#include <llsf_sps/mps_deliver.h>

#include <boost/bind/bind.hpp>
#include <boost/format.hpp>
#if BOOST_ASIO_VERSION < 100601
#  include <csignal>
#endif
#ifdef HAVE_MONGODB
#	include <bsoncxx/builder/basic/document.hpp>
#	include <bsoncxx/document/value.hpp>
#	include <bsoncxx/exception/exception.hpp>
#	include <bsoncxx/json.hpp>
#	include <mongocxx/client.hpp>
#	include <mongocxx/exception/operation_exception.hpp>
#	include <mongodb_log/mongodb_log_logger.h>
#	include <mongodb_log/mongodb_log_protobuf.h>
#endif
#ifdef HAVE_AVAHI
#  include <netcomm/dns-sd/avahi_thread.h>
#  include <netcomm/utils/resolver.h>
#  include <netcomm/service_discovery/service.h>
#else
#  include <netcomm/service_discovery/dummy_service_browser.h>
#  include <netcomm/service_discovery/dummy_service_publisher.h>
#endif

#include <rest_api/webview_server.h>

#include <string>

using namespace llsf_sps;
using namespace protobuf_comm;
using namespace protobuf_clips;
using namespace llsf_utils;

#ifdef HAVE_MONGODB
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
#endif

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#if BOOST_ASIO_VERSION < 100601
LLSFRefBox *g_refbox = NULL;
static void handle_signal(int signum)
{
  if (g_refbox) {
    g_refbox->handle_signal(boost::system::errc::make_error_code(boost::system::errc::success),
			    signum);
  }
}
#endif

/** @class LLSFRefBox "refbox.h"
 * LLSF referee box main application.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param argc number of arguments passed
 * @param argv array of arguments
 */
LLSFRefBox::LLSFRefBox(int argc, char **argv)
  : clips_mutex_(fawkes::Mutex::RECURSIVE), timer_(io_service_)
{
  pb_comm_ = NULL;

  config_ = new YamlConfiguration(CONFDIR);
  config_->load("config.yaml");

  cfg_clips_dir_ = std::string(BASEDIR) + "/src/games/rcll/";

  try {
    cfg_timer_interval_ = config_->get_uint("/llsfrb/clips/timer-interval");
  } catch (fawkes::Exception &e) {
    delete config_;
    throw;
  }

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
  } catch (fawkes::Exception &e) {} // ignored, use default

  MultiLogger *mlogger = new MultiLogger();
  mlogger->add_logger(new ConsoleLogger(log_level_));
  try {
    std::string logfile = config_->get_string("/llsfrb/log/general");
    mlogger->add_logger(new FileLogger(logfile.c_str(), log_level_));
  } catch (fawkes::Exception &e) {} // ignored, use default
  logger_ = mlogger;


  cfg_machine_assignment_ = ASSIGNMENT_2014;
  try {
    std::string m_ass_str = config_->get_string("/llsfrb/game/machine-assignment");
    if (m_ass_str == "2013") {
      cfg_machine_assignment_ = ASSIGNMENT_2013;
    } else if (m_ass_str == "2014") {
      cfg_machine_assignment_ = ASSIGNMENT_2014;
    } else {
      logger_->log_warn("RefBox", "Invalid machine assignment '%s', using 2014",
			m_ass_str.c_str());
      cfg_machine_assignment_ = ASSIGNMENT_2014;
    }
  } catch (fawkes::Exception &e) {} // ignored, use default
  logger_->log_info("RefBox", "Using %s machine assignment",
		    (cfg_machine_assignment_ == ASSIGNMENT_2013) ? "2013" : "2014");

  try {
    sps_ = NULL;
    if (config_->get_bool("/llsfrb/sps/enable")) {
      logger_->log_info("RefBox", "Connecting to SPS");
      bool test_lights = true;
      try {
	test_lights = config_->get_bool("/llsfrb/sps/test-lights");
      } catch (fawkes::Exception &e) {} // ignore, use default

      if (config_->exists("/llsfrb/sps/hosts") && cfg_machine_assignment_ == ASSIGNMENT_2014) {
	sps_ = new SPSComm(config_->get_strings("/llsfrb/sps/hosts"),
			   config_->get_uint("/llsfrb/sps/port"),
			   config_->get_string("/llsfrb/sps/machine-type"));
      }

      sps_->reset_lights();
      sps_->reset_rfids();
      if (test_lights) {
	sps_->test_lights();
      }
    }
  } catch (fawkes::Exception &e) {
    logger_->log_warn("RefBox", "Cannot connect to SPS, running without");
    delete sps_;
    sps_ = NULL;
  }


  try {
    mps_ = NULL;
    if (config_->get_bool("/llsfrb/mps/enable")) {
      mps_ = new MPSRefboxInterface("MPSInterface");

      std::string prefix = "/llsfrb/mps/stations/";

      std::set<std::string> mps_configs;
      std::set<std::string> ignored_mps_configs;

#if __cplusplus >= 201103L
      std::unique_ptr<Configuration::ValueIterator> i(config_->search(prefix.c_str()));
#else
      std::auto_ptr<Configuration::ValueIterator> i(config_->search(prefix.c_str()));
#endif
      while (i->next()) {

	std::string cfg_name = std::string(i->path()).substr(prefix.length());
	cfg_name = cfg_name.substr(0, cfg_name.find("/"));

#ifdef HAVE_MONGODB
	cfg_mongodb_enabled_ = false;
	try {
		cfg_mongodb_enabled_ = config_->get_bool("/llsfrb/mongodb/enable");
	} catch (fawkes::Exception &e) {
	} // ignore, use default

	if (cfg_mongodb_enabled_) {
		cfg_mongodb_hostport_     = config_->get_string("/llsfrb/mongodb/hostport");
		std::string mdb_text_log  = config_->get_string("/llsfrb/mongodb/collections/text-log");
		std::string mdb_clips_log = config_->get_string("/llsfrb/mongodb/collections/clips-log");
		std::string mdb_protobuf  = config_->get_string("/llsfrb/mongodb/collections/protobuf");
		mlogger->add_logger(new MongoDBLogLogger(cfg_mongodb_hostport_, mdb_text_log));

		clips_logger_->add_logger(new MongoDBLogLogger(cfg_mongodb_hostport_, mdb_clips_log));

		mongodb_protobuf_ = new MongoDBLogProtobuf(cfg_mongodb_hostport_, mdb_protobuf);

		auto client = mongocxx::client{mongocxx::uri{"mongodb://" + cfg_mongodb_hostport_}};
		database_   = client["rcll"];

		setup_clips_mongodb();

		pb_comm_->server()->signal_received().connect(
		  boost::bind(&LLSFRefBox::handle_server_client_msg, this, _1, _2, _3, _4));
		pb_comm_->server()->signal_receive_failed().connect(
		  boost::bind(&LLSFRefBox::handle_server_client_fail, this, _1, _2, _3, _4));

	  std::string cfg_prefix = prefix + cfg_name + "/";

	  printf("Config: %s  prefix %s\n", cfg_name.c_str(), cfg_prefix.c_str());

	  bool active = true;
	  try {
	    active = config_->get_bool((cfg_prefix + "active").c_str());
	  } catch (Exception &e) {} // ignored, assume enabled

	  if (active) {

 	    std::string mpstype = config_->get_string((cfg_prefix + "type").c_str());
	    std::string mpsip = config_->get_string((cfg_prefix + "host").c_str());
	    unsigned int port = config_->get_uint((cfg_prefix + "port").c_str());

	    if(mpstype == "BS") {
	      logger_->log_info("RefBox", "Adding BS %s:%u", mpsip.c_str(), port);
	      MPSIncomingStation *is = new MPSIncomingStation(mpsip.c_str(), port, cfg_name.c_str());
	      mps_->insertMachine(cfg_name, is, is);
	    }
	    else if(mpstype == "CS") {
	      logger_->log_info("RefBox", "Adding CS %s:%u", mpsip.c_str(), port, cfg_name.c_str());
	      MPSPickPlace1 *pp1 = new MPSPickPlace1(mpsip.c_str(), port, cfg_name.c_str());
	      mps_->insertMachine(cfg_name, pp1, pp1);
	    }
	    else if(mpstype == "RS") {
	      logger_->log_info("RefBox", "Adding RS %s:%u", mpsip.c_str(), port);
	      MPSPickPlace2 *pp2 = new MPSPickPlace2(mpsip.c_str(), port, cfg_name.c_str());
	      mps_->insertMachine(cfg_name, pp2, pp2);
	    }
	    else if(mpstype == "DS") {
	      logger_->log_info("RefBox", "Adding DS %s:%u", mpsip.c_str(), port);
	      MPSDeliver *del = new MPSDeliver(mpsip.c_str(), port, cfg_name.c_str());
	      mps_->insertMachine(cfg_name, del, del);
	    }
	    else {
	      throw fawkes::Exception("this type wont match");
	    }
	    mps_configs.insert(cfg_name);
	  } else {
	    ignored_mps_configs.insert(cfg_name);
	  }
	}
      }
    }
  } catch (Exception &e) {
    throw;
  }


  clips_ = new CLIPS::Environment();
  setup_protobuf_comm();
  setup_clips();

  mps_placing_generator_ = std::shared_ptr<mps_placing_clips::MPSPlacingGenerator>(
        new mps_placing_clips::MPSPlacingGenerator(clips_, clips_mutex_)
        );

  mlogger->add_logger(new NetworkLogger(pb_comm_->server(), log_level_));

 #ifdef HAVE_MONGODB
  cfg_mongodb_enabled_ = false;
  try {
    cfg_mongodb_enabled_ = config_->get_bool("/llsfrb/mongodb/enable");
  } catch (fawkes:: Exception &e) {} // ignore, use default

  if (cfg_mongodb_enabled_) {
    cfg_mongodb_hostport_     = config_->get_string("/llsfrb/mongodb/hostport");
    std::string mdb_text_log  = config_->get_string("/llsfrb/mongodb/collections/text-log");
    std::string mdb_clips_log = config_->get_string("/llsfrb/mongodb/collections/clips-log");
    std::string mdb_protobuf  = config_->get_string("/llsfrb/mongodb/collections/protobuf");
    mlogger->add_logger(new MongoDBLogLogger(cfg_mongodb_hostport_, mdb_text_log));

    clips_logger_->add_logger(new MongoDBLogLogger(cfg_mongodb_hostport_, mdb_clips_log));

    mongodb_protobuf_ = new MongoDBLogProtobuf(cfg_mongodb_hostport_, mdb_protobuf);


    mongo::DBClientConnection *conn =
      new mongo::DBClientConnection(/* auto reconnect */ true);
    mongodb_ = conn;
    std::string errmsg;
    if (! conn->connect(cfg_mongodb_hostport_, errmsg)) {
      throw fawkes::Exception("Could not connect to MongoDB at %s: %s",
			      cfg_mongodb_hostport_.c_str(), errmsg.c_str());
    }

    setup_clips_mongodb();

    pb_comm_->server()->signal_received()
      .connect(boost::bind(&LLSFRefBox::handle_server_client_msg, this, _1, _2, _3, _4));
    pb_comm_->server()->signal_receive_failed()
      .connect(boost::bind(&LLSFRefBox::handle_server_client_fail, this, _1, _2, _3, _4));

    pb_comm_->signal_server_sent()
      .connect(boost::bind(&LLSFRefBox::handle_server_sent_msg, this, _1, _2));
    pb_comm_->signal_client_sent()
      .connect(boost::bind(&LLSFRefBox::handle_client_sent_msg, this, _1, _2, _3));
    pb_comm_->signal_peer_sent()
      .connect(boost::bind(&LLSFRefBox::handle_peer_sent_msg, this, _2));
  }
#endif

  start_clips();

#ifdef HAVE_MONGODB
  // we can do this only after CLIPS was started as it initiates the private peers
  if (cfg_mongodb_enabled_) {
    const std::map<long int, protobuf_comm::ProtobufBroadcastPeer *> &peers =
      pb_comm_->peers();
    for (auto p : peers) {
      p.second->signal_received()
	.connect(boost::bind(&LLSFRefBox::handle_peer_msg, this, _1, _2, _3, _4));
    }
  }
#endif


#ifdef HAVE_AVAHI
  unsigned int refbox_port = config_->get_uint("/llsfrb/comm/server-port");
  avahi_thread_ = new fawkes::AvahiThread();
  service_publisher_ = avahi_thread_;
  service_browser_   = avahi_thread_;
  avahi_thread_->start();
  nnresolver_   = new fawkes::NetworkNameResolver(avahi_thread_);
  fawkes::NetworkService *refbox_service =
    new fawkes::NetworkService(nnresolver_, "RefBox on %h", "_refbox._tcp", refbox_port);
  avahi_thread_->publish_service(refbox_service);
  delete refbox_service;
#else
	service_publisher_ = new DummyServicePublisher();
	service_browser_   = new DummyServiceBrowser();
	nnresolver_        = new NetworkNameResolver();
#endif

  try{
    rest_api_thread_ = new WebviewServer(false,
		  nnresolver_,
		  service_publisher_,
		  service_browser_,
		  clips_mutex_,
		  clips_,
		  config_,logger_);
    rest_api_thread_ -> init();
    rest_api_thread_ -> start();
      logger_->log_info("RefBox", " rest-api server started ");
   } catch(Exception &e){
      logger_->log_info("RefBox", "Could not start rest-api");
      logger_->log_error("Exception: ", e.what() );
   }

}

/** Destructor. */
LLSFRefBox::~LLSFRefBox()
{
  timer_.cancel();

  rest_api_thread_->cancel();
  rest_api_thread_->join();
  delete rest_api_thread_;

#ifdef HAVE_AVAHI
  avahi_thread_->cancel();
  avahi_thread_->join();
  delete avahi_thread_;
#endif
  delete nnresolver_;
  delete service_publisher_;
  delete service_browser_;

  //std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
  {
    fawkes::MutexLocker lock(&clips_mutex_);
    clips_->assert_fact("(finalize)");
    clips_->refresh_agenda();
    clips_->run();

    finalize_clips_logger(clips_->cobj());
  }

  mps_placing_generator_.reset();

  delete pb_comm_;
  delete config_;
  delete sps_;
  delete clips_;
  delete logger_;
  delete clips_logger_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}


void
LLSFRefBox::setup_protobuf_comm()
{
  try {
    std::vector<std::string> proto_dirs;
    try {
      proto_dirs = config_->get_strings("/llsfrb/comm/protobuf-dirs");
      if (proto_dirs.size() > 0) {
	for (size_t i = 0; i < proto_dirs.size(); ++i) {
	  std::string::size_type pos;
	  if ((pos = proto_dirs[i].find("@BASEDIR@")) != std::string::npos) {
	    proto_dirs[i].replace(pos, 9, BASEDIR);
	  }
	  if ((pos = proto_dirs[i].find("@RESDIR@")) != std::string::npos) {
	    proto_dirs[i].replace(pos, 8, RESDIR);
	  }
	  if ((pos = proto_dirs[i].find("@CONFDIR@")) != std::string::npos) {
	    proto_dirs[i].replace(pos, 9, CONFDIR);
	  }

	  if (proto_dirs[i][proto_dirs.size()-1] != '/') {
	    proto_dirs[i] += "/";
	  }
	  //logger_->log_warn("RefBox", "DIR: %s", proto_dirs[i].c_str());
	}
      }
    } catch (fawkes::Exception &e) {} // ignore, use default

    if (proto_dirs.empty()) {
      pb_comm_ = new ClipsProtobufCommunicator(clips_, clips_mutex_);
    } else {
      pb_comm_ = new ClipsProtobufCommunicator(clips_, clips_mutex_, proto_dirs);
    }

    pb_comm_->enable_server(config_->get_uint("/llsfrb/comm/server-port"));

    MessageRegister &mr_server = pb_comm_->message_register();
    if (! mr_server.load_failures().empty()) {
      MessageRegister::LoadFailMap::const_iterator e = mr_server.load_failures().begin();
      std::string errstr = e->first + " (" + e->second + ")";
      for (++e; e != mr_server.load_failures().end(); ++e) {
	errstr += std::string(", ") + e->first + " (" + e->second + ")";
      }
      logger_->log_warn("RefBox", "Failed to load some message types: %s", errstr.c_str());
    }

  } catch (std::runtime_error &e) {
    delete config_;
    delete sps_;
    delete pb_comm_;
    throw;
  }
}

void
LLSFRefBox::setup_clips()
{
  fawkes::MutexLocker lock(&clips_mutex_);

  logger_->log_info("RefBox", "Creating CLIPS environment");
  MultiLogger *mlogger = new MultiLogger();
  mlogger->add_logger(new ConsoleLogger(log_level_));
  try {
    std::string logfile = config_->get_string("/llsfrb/log/clips");
    mlogger->add_logger(new FileLogger(logfile.c_str(), Logger::LL_DEBUG));
  } catch (fawkes::Exception &e) {} // ignored, use default

  clips_logger_ = mlogger;

  bool simulation = false;
  try {
    simulation = config_->get_bool("/llsfrb/simulation/enabled");
  } catch (Exception &e) {} // ignore, use default

  init_clips_logger(clips_->cobj(), logger_, clips_logger_);

  std::string defglobal_ver =
    boost::str(boost::format("(defglobal\n"
			     "  ?*VERSION-MAJOR* = %u\n"
			     "  ?*VERSION-MINOR* = %u\n"
			     "  ?*VERSION-MICRO* = %u\n"
			     ")")
	       % FAWKES_VERSION_MAJOR
	       % FAWKES_VERSION_MINOR
	       % FAWKES_VERSION_MICRO);

  clips_->build(defglobal_ver);

  clips_->add_function("get-clips-dirs", sigc::slot<CLIPS::Values>(sigc::mem_fun(*this, &LLSFRefBox::clips_get_clips_dirs)));
  clips_->add_function("now", sigc::slot<CLIPS::Values>(sigc::mem_fun(*this, &LLSFRefBox::clips_now)));
  clips_->add_function("load-config", sigc::slot<void, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_load_config)));
  clips_->add_function("config-path-exists", sigc::slot<CLIPS::Value, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_config_path_exists)));
  clips_->add_function("config-get-bool", sigc::slot<CLIPS::Value, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_config_get_bool)));
  clips_->add_function("sps-set-signal", sigc::slot<void, std::string, std::string, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_sps_set_signal)));

  if (mps_ && ! simulation) {
    clips_->add_function("mps-bs-dispense", sigc::slot<void, std::string, std::string, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_bs_dispense)));
    clips_->add_function("mps-ds-process", sigc::slot<void, std::string, int>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_ds_process)));
    clips_->add_function("mps-rs-mount-ring", sigc::slot<void, std::string, int>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_rs_mount_ring)));
    clips_->add_function("mps-cs-process", sigc::slot<void, std::string, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_cs_process)));
    clips_->add_function("mps-set-light", sigc::slot<void, std::string, std::string, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_set_light)));
    clips_->add_function("mps-set-lights", sigc::slot<void, std::string, std::string, std::string, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_set_lights)));
    clips_->add_function("mps-reset", sigc::slot<void, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_reset)));
    clips_->add_function("mps-reset-base-counter", sigc::slot<void, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_reset_base_counter)));
    clips_->add_function("mps-deliver", sigc::slot<void, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_mps_deliver)));
  }

  clips_->signal_periodic().connect(sigc::mem_fun(*this, &LLSFRefBox::handle_clips_periodic));

}

void
LLSFRefBox::start_clips()
{
  fawkes::MutexLocker lock(&clips_mutex_);

  if (!clips_->batch_evaluate(cfg_clips_dir_ + "init.clp")) {
    logger_->log_warn("RefBox", "Failed to initialize CLIPS environment, batch file failed.");
    throw fawkes::Exception("Failed to initialize CLIPS environment, batch file failed.");
  }

  clips_->assert_fact("(init)");
  clips_->refresh_agenda();
  clips_->run();
}

void
LLSFRefBox::handle_clips_periodic()
{
  std::queue<int> to_erase;
  std::map<long int, CLIPS::Fact::pointer>::iterator f;

  for (f = clips_msg_facts_.begin(); f != clips_msg_facts_.end(); ++f) {
    if (f->second->refcount() == 1) {
      //logger_->log_info("RefBox", "Fact %li can be erased", f->second->index());
      to_erase.push(f->first);
    }
  }
  while (! to_erase.empty()) {
    long int index = to_erase.front();
    CLIPS::Fact::pointer &f = clips_msg_facts_[index];
    CLIPS::Value v = f->slot_value("ptr")[0];
    void *ptr = v.as_address();
    delete static_cast<std::shared_ptr<google::protobuf::Message> *>(ptr);
    clips_msg_facts_.erase(index);
    to_erase.pop();
  }
}


CLIPS::Values
LLSFRefBox::clips_now()
{
  CLIPS::Values rv;
  struct timeval tv;
  gettimeofday(&tv, 0);
  rv.push_back(tv.tv_sec);
  rv.push_back(tv.tv_usec);
  return rv;
}


CLIPS::Values
LLSFRefBox::clips_get_clips_dirs()
{
  CLIPS::Values rv;
  rv.push_back(cfg_clips_dir_);
  return rv;
}

void
LLSFRefBox::clips_load_config(std::string cfg_prefix)
{
  std::shared_ptr<Configuration::ValueIterator> v(config_->search(cfg_prefix.c_str()));
  while (v->next()) {
    std::string type = "";
    std::string value = v->get_as_string();

    if      (v->is_uint())   type = "UINT";
    else if (v->is_int())    type = "INT";
    else if (v->is_float())  type = "FLOAT";
    else if (v->is_bool())   type = "BOOL";
    else if (v->is_string()) {
      type = "STRING";
      if (! v->is_list()) {
	value = std::string("\"") + value + "\"";
      }
    } else {
      logger_->log_warn("RefBox", "Config value at '%s' of unknown type '%s'",
	     v->path(), v->type());
    }

    if (v->is_list()) {
      //logger_->log_info("RefBox", "(confval (path \"%s\") (type %s) (is-list TRUE) (list-value %s))",
      //       v->path(), type.c_str(), value.c_str());
      clips_->assert_fact_f("(confval (path \"%s\") (type %s) (is-list TRUE) (list-value %s))",
			    v->path(), type.c_str(), value.c_str());
    } else {
      //logger_->log_info("RefBox", "(confval (path \"%s\") (type %s) (value %s))",
      //       v->path(), type.c_str(), value.c_str());
      clips_->assert_fact_f("(confval (path \"%s\") (type %s) (value %s))",
			    v->path(), type.c_str(), value.c_str());
    }
  }
}

CLIPS::Value
LLSFRefBox::clips_config_path_exists(std::string path)
{
  return CLIPS::Value(config_->exists(path.c_str()) ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

CLIPS::Value
LLSFRefBox::clips_config_get_bool(std::string path)
{
  try {
    bool v = config_->get_bool(path.c_str());
    return CLIPS::Value(v ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
  } catch (Exception &e) {
    return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
  }
}


void
LLSFRefBox::clips_sps_set_signal(std::string machine, std::string light, std::string state)
{
  if (! sps_)  return;
  try {
    unsigned int m = to_machine(machine, cfg_machine_assignment_);
    sps_->set_light(m, light, state);
  } catch (fawkes::Exception &e) {
    logger_->log_warn("RefBox", "Failed to set signal: %s", e.what());
  }
}

void
LLSFRefBox::clips_mps_reset(std::string machine)
{
  logger_->log_info("MPS", "Resetting machine %s", machine.c_str());

  if (! mps_)  return;
  MPS *station;
  station = mps_->get_station(machine, station);
  if (station) {
    station->resetMachine();
    station->clearRegister();
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}

void
LLSFRefBox::clips_mps_reset_base_counter(std::string machine)
{
  logger_->log_info("MPS", "Resetting machine %s", machine.c_str());

  if (! mps_)  return;
  MPSPickPlace2 *station;
  station = mps_->get_station(machine, station);
  if (station) {
    station->resetCounterSlide();
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}


void
LLSFRefBox::clips_mps_deliver(std::string machine)
{
  logger_->log_info("MPS", "Delivering on %s", machine.c_str());

  if (! mps_)  return;
  MPS *station;
  station = mps_->get_station(machine, station);
  if (station) {
    station->deliverProduct();
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}

void
LLSFRefBox::clips_mps_bs_dispense(std::string machine, std::string color, std::string side)
{
  logger_->log_info("MPS", "Dispense %s: %s at %s",
		    machine.c_str(), color.c_str(), side.c_str());
  if (! mps_)  return;
  MPSIncomingStation *station;
  station = mps_->get_station(machine, station);
  if (station) {
    int color_id = 0;
    if (color == "BASE_RED") {
      color_id = 1;
    } else if (color == "BASE_SILVER") {
      color_id = 2;
    } else if (color == "BASE_BLACK") {
      color_id = 3;
    } else {
      logger_->log_error("MPS", "Invalid color %s", color.c_str());
      return;
    }

    int side_id = 0;
    if (side == "INPUT") {
      side_id = 2;
    } else if (side == "OUTPUT") {
      side_id = 1;
    } else {
      logger_->log_error("MPS", "Invalid side %s", side.c_str());
      return;
    }

    station->getCap(color_id, side_id);
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}


void
LLSFRefBox::clips_mps_ds_process(std::string machine, int slide)
{
  logger_->log_info("MPS", "Processing on %s: slide %d",
		    machine.c_str(), slide);
  if (! mps_)  return;
  MPSDeliver *station;
  station = mps_->get_station(machine, station);
  if (station) {
    station->sendDeliver(slide);
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}

void
LLSFRefBox::clips_mps_rs_mount_ring(std::string machine, int slide)
{
  logger_->log_info("MPS", "Mount ring on %s: slide %d",
		    machine.c_str(), slide);
  if (! mps_)  return;
  MPSPickPlace2 *station;
  station = mps_->get_station(machine, station);
  if (station) {
    station->produceRing(slide);
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}


void
LLSFRefBox::clips_mps_cs_process(std::string machine, std::string operation)
{
  logger_->log_info("MPS", "%s on %s",
		    operation.c_str(), machine.c_str());
  if (! mps_)  return;
  MPSPickPlace1 *station;
  station = mps_->get_station(machine, station);
  if (station) {
    if (operation == "RETRIEVE_CAP") {
      station->produceEnd(2);
    } else if (operation == "MOUNT_CAP") {
      station->produceEnd(1);
    } else {
      logger_->log_error("MPS", "Invalid operation '%s' on %s",
			 operation.c_str(), machine.c_str());
    }
  } else {
    logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}


void
LLSFRefBox::clips_mps_set_light(std::string machine, std::string color, std::string state)
{
  //logger_->log_info("MPS", "Set light %s: %s to %s",
  //		    machine.c_str(), color.c_str(), state.c_str());

  if (! mps_)  return;
  MPS *station;
  station = mps_->get_station(machine, station);
  if (station) {
    int color_id = 0;
    if (color == "RED") {
      color_id = 1;
    } else if (color == "YELLOW") {
      color_id = 2;
    } else if (color == "GREEN") {
      color_id = 3;
    } else {
      logger_->log_error("MPS", "Invalid color %s", color.c_str());
      return;
    }

    int state_id = 0;
    int blink_id = 0;
    if (state == "ON") {
      state_id = 1;
      blink_id = 0;
    } else if (state == "BLINK") {
      state_id = 1;
      blink_id = 1;
    } else if (state == "OFF") {
      state_id = 0;
      blink_id = 0;
    } else {
      logger_->log_error("MPS", "Invalid state %s", state.c_str());
      return;
    }

    //printf("Set light %i %i %i\n", color_id, state_id, blink_id);
    station->setLight(color_id, state_id, blink_id);

  } else {
    //logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}

void
LLSFRefBox::clips_mps_set_lights(std::string machine, std::string red_state,
                                 std::string yellow_state, std::string green_state)
{
  //logger_->log_info("MPS", "Set light on %s: red %s yellow %s  green %s",
  //                  machine.c_str(), red_state.c_str(), yellow_state.c_str(), green_state.c_str());

  if (! mps_)  return;
  MPS *station;
  station = mps_->get_station(machine, station);
  if (station) {

	  // 0 is off, 1 is on, 2 is blink
	  int lights[3];

    if (red_state == "ON") {
	    lights[0] = 1;
    } else if (red_state == "BLINK") {
	    lights[0] = 2;
    } else {
	    lights[0] = 0;
    }

    if (yellow_state == "ON") {
	    lights[1] = 1;
    } else if (yellow_state == "BLINK") {
	    lights[1] = 2;
    } else {
	    lights[1] = 0;
    }

    if (green_state == "ON") {
	    lights[2] = 1;
    } else if (green_state == "BLINK") {
	    lights[2] = 2;
    } else {
	    lights[2] = 0;
    }

    //printf("Set light %i %i %i\n", color_id, state_id, blink_id);
    station->setAllLights(lights);

  } else {
    //logger_->log_error("MPS", "Invalid station %s", machine.c_str());
    return;
  }
}

#ifdef HAVE_MONGODB

/** Handle message that came from a client.
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message
 */
void
LLSFRefBox::handle_server_client_msg(ProtobufStreamServer::ClientID client,
				     uint16_t component_id, uint16_t msg_type,
				     std::shared_ptr<google::protobuf::Message> msg)
{
	document meta{};
	meta.append(kvp("direction", "inbound"));
	meta.append(kvp("via", "server"));
	meta.append(kvp("component_id", component_id));
	meta.append(kvp("msg_type", msg_type));
	meta.append(kvp("client_id", (int32_t)client));
	mongodb_protobuf_->write(*msg, meta.view());
}

/** Handle message that came from a client.
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message
 */
void
LLSFRefBox::handle_peer_msg(boost::asio::ip::udp::endpoint &endpoint,
			    uint16_t component_id, uint16_t msg_type,
			    std::shared_ptr<google::protobuf::Message> msg)
{
	document meta{};
	meta.append(kvp("direction", "inbound"));
	meta.append(kvp("via", "peer"));
	meta.append(kvp("endpoint-host", endpoint.address().to_string()));
	meta.append(kvp("endpoint-port", endpoint.port()));
	meta.append(kvp("component_id", component_id));
	meta.append(kvp("msg_type", msg_type));
	mongodb_protobuf_->write(*msg, meta.view());
}

/** Handle server reception failure
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message string
 */
void
LLSFRefBox::handle_server_client_fail(ProtobufStreamServer::ClientID client,
				      uint16_t component_id, uint16_t msg_type,
				      std::string msg)
{
}


void
LLSFRefBox::add_comp_type(google::protobuf::Message &m, document *doc)
{
	const google::protobuf::Descriptor *    desc     = m.GetDescriptor();
	const google::protobuf::EnumDescriptor *enumdesc = desc->FindEnumTypeByName("CompType");
	if (!enumdesc)
		return;
	const google::protobuf::EnumValueDescriptor *compdesc = enumdesc->FindValueByName("COMP_ID");
	const google::protobuf::EnumValueDescriptor *msgtdesc = enumdesc->FindValueByName("MSG_TYPE");
	if (!compdesc || !msgtdesc)
		return;
	int comp_id  = compdesc->number();
	int msg_type = msgtdesc->number();
	doc->append(kvp("component_id", comp_id));
	doc->append(kvp("msg_type", msg_type));
}

/** Handle message that was sent to a server client.
 * @param client client ID
 * @param msg the message
 */
void
LLSFRefBox::handle_server_sent_msg(ProtobufStreamServer::ClientID client,
				   std::shared_ptr<google::protobuf::Message> msg)
{
	document meta{};
	meta.append(kvp("direction", "outbound"));
	meta.append(kvp("via", "server"));
	meta.append(kvp("client_id", (int32_t)client));
	add_comp_type(*msg, &meta);
	mongodb_protobuf_->write(*msg, meta.view());
}

/** Handle message that was sent with a client.
 * @param host host of the endpoint sent to
 * @param port port of the endpoint sent to
 * @param msg the message
 */
void
LLSFRefBox::handle_client_sent_msg(std::string host, unsigned short int port,
				   std::shared_ptr<google::protobuf::Message> msg)
{
	document meta{};
	meta.append(kvp("direction", "outbound"));
	meta.append(kvp("via", "client"));
	meta.append(kvp("host", host));
	meta.append(kvp("port", port));
	add_comp_type(*msg, &meta);
	mongodb_protobuf_->write(*msg, meta.view());
}

/** Setup MongoDB related CLIPS functions. */
void
LLSFRefBox::setup_clips_mongodb()
{
	fawkes::MutexLocker lock(&clips_mutex_);

	clips_->add_function("bson-create",
	                     sigc::slot<CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_create)));
	clips_->add_function("bson-parse",
	                     sigc::slot<CLIPS::Value, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_parse)));
	clips_->add_function("bson-destroy",
	                     sigc::slot<void, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_destroy)));
	clips_->add_function("bson-append",
	                     sigc::slot<void, void *, std::string, CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_append)));
	clips_->add_function("bson-append-array",
	                     sigc::slot<void, void *, std::string, CLIPS::Values>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_append_array)));
	clips_->add_function("bson-array-start",
	                     sigc::slot<CLIPS::Value, void *, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_array_start)));
	clips_->add_function("bson-array-finish",
	                     sigc::slot<void, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_array_finish)));
	clips_->add_function("bson-array-append",
	                     sigc::slot<void, void *, CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_array_append)));

	clips_->add_function("bson-append-time",
	                     sigc::slot<void, void *, std::string, CLIPS::Values>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_append_time)));
	clips_->add_function("bson-tostring",
	                     sigc::slot<std::string, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_tostring)));
	clips_->add_function("mongodb-insert",
	                     sigc::slot<void, std::string, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_insert)));
	clips_->add_function("mongodb-upsert",
	                     sigc::slot<void, std::string, void *, CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_upsert)));
	clips_->add_function("mongodb-update",
	                     sigc::slot<void, std::string, void *, CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_update)));
	clips_->add_function("mongodb-replace",
	                     sigc::slot<void, std::string, void *, CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_replace)));
	clips_->add_function("mongodb-query",
	                     sigc::slot<CLIPS::Value, std::string, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_query)));
	clips_->add_function("mongodb-query-sort",
	                     sigc::slot<CLIPS::Value, std::string, void *, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_query_sort)));
	/*
	clips_->add_function("mongodb-cursor-destroy",
	                     sigc::slot<void, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_cursor_destroy)));
	clips_->add_function("mongodb-cursor-more",
	                     sigc::slot<CLIPS::Value, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_cursor_more)));
	clips_->add_function("mongodb-cursor-next",
	                     sigc::slot<CLIPS::Value, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_cursor_next)));
  */
	clips_->add_function("bson-field-names",
	                     sigc::slot<CLIPS::Values, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_field_names)));
	clips_->add_function("bson-get",
	                     sigc::slot<CLIPS::Value, void *, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_get)));
	clips_->add_function("bson-get-array",
	                     sigc::slot<CLIPS::Values, void *, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_get_array)));
	clips_->add_function("bson-get-time",
	                     sigc::slot<CLIPS::Values, void *, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_get_time)));

	clips_->build("(deffacts have-feature-mongodb (have-feature MongoDB))");
}

/** Handle message that was sent to a server client.
 * @param client client ID
 * @param msg the message
 */
void
LLSFRefBox::handle_peer_sent_msg(std::shared_ptr<google::protobuf::Message> msg)
{
	document meta{};
	meta.append(kvp("direction", "outbound"));
	meta.append(kvp("via", "peer"));
	add_comp_type(*msg, &meta);
	mongodb_protobuf_->write(*msg, meta.view());
}


CLIPS::Value
LLSFRefBox::clips_bson_create()
{
	return CLIPS::Value(new bsoncxx::builder::basic::document());
}

CLIPS::Value
LLSFRefBox::clips_bson_parse(std::string document)
{
	auto b = new bsoncxx::builder::basic::document();
	try {
		b->append(bsoncxx::builder::concatenate(bsoncxx::from_json(document)));
	} catch (bsoncxx::exception &e) {
		logger_->log_error("MongoDB", "Parsing JSON doc failed: %s\n%s", e.what(), document.c_str());
	}
	return CLIPS::Value(b);
}

void
LLSFRefBox::clips_bson_destroy(void *bson)
{
	auto b = static_cast<bsoncxx::builder::basic::document *>(bson);
	delete b;
}

std::string
LLSFRefBox::clips_bson_tostring(void *bson)
{
	auto b = static_cast<bsoncxx::builder::basic::document *>(bson);
	return bsoncxx::to_json(b->view());
}

void
LLSFRefBox::clips_bson_append(void *bson, std::string field_name, CLIPS::Value value)
{
	try {
		auto b = static_cast<document *>(bson);
		switch (value.type()) {
		case CLIPS::TYPE_FLOAT: b->append(kvp(field_name, value.as_float())); break;

		case CLIPS::TYPE_INTEGER:
			b->append(kvp(field_name, static_cast<int64_t>(value.as_integer())));
			break;

		case CLIPS::TYPE_SYMBOL:
		case CLIPS::TYPE_INSTANCE_NAME:
		case CLIPS::TYPE_STRING: b->append(kvp(field_name, value.as_string())); break;
		case CLIPS::TYPE_EXTERNAL_ADDRESS: {
			auto subb = static_cast<document *>(value.as_address());
			b->append(kvp(field_name, subb->view()));
		} break;

		default:
			logger_->log_warn("RefBox", "Tried to add unknown type to BSON field %s", field_name.c_str());
			break;
		}
	} catch (bsoncxx::exception &e) {
		logger_->log_error("MongoDB",
		                   "Failed to append array value to field %s: %s",
		                   field_name.c_str(),
		                   e.what());
	}
}


void
LLSFRefBox::clips_bson_append_array(void *bson,
				    std::string field_name, CLIPS::Values values)
{
	try {
		auto b = static_cast<document *>(bson);

		b->append(kvp(field_name, [&](bsoncxx::builder::basic::sub_array array) {
			for (auto value : values) {
				switch (value.type()) {
				case CLIPS::TYPE_FLOAT: array.append(value.as_float()); break;

				case CLIPS::TYPE_INTEGER: array.append(static_cast<int64_t>(value.as_integer())); break;

				case CLIPS::TYPE_SYMBOL:
				case CLIPS::TYPE_STRING:
				case CLIPS::TYPE_INSTANCE_NAME: array.append(value.as_string()); break;

				case CLIPS::TYPE_EXTERNAL_ADDRESS: {
					auto subb = static_cast<document *>(value.as_address());
					array.append(subb->view());
				} break;

				default:
					logger_->log_warn("MongoDB",
					                  "Tried to add unknown type to BSON array field %s",
					                  field_name.c_str());
					break;
				}
			}
		}));
	} catch (bsoncxx::exception &e) {
		logger_->log_error("MongoDB",
		                   "Failed to append array value to field %s: %s",
		                   field_name.c_str(),
		                   e.what());
	}
}

CLIPS::Value
LLSFRefBox::clips_bson_array_start(void *bson, std::string field_name)
{
	// With the new libmongocxx, we can no longer create an open array as
	// sub-field of another document.
	throw Exception("Not implemented");
}


void
LLSFRefBox::clips_bson_array_finish(void *barr)
{
	// With the new libmongocxx, we can no longer create an open array as
	// sub-field of another document.
	throw Exception("Not implemented");
}

void
LLSFRefBox::clips_bson_array_append(void *barr, CLIPS::Value value)
{
	// With the new libmongocxx, we can no longer create an open array as
	// sub-field of another document.
	throw Exception("Not implemented");
}


void
LLSFRefBox::clips_bson_append_time(void *bson, std::string field_name, CLIPS::Values time)
{
	if (time.size() != 2) {
		logger_->log_warn("MongoDB", "Invalid time, %zu instead of 2 entries", time.size());
		return;
	}
	if (time[0].type() != CLIPS::TYPE_INTEGER || time[1].type() != CLIPS::TYPE_INTEGER) {
		logger_->log_warn("MongoDB", "Invalid time, type mismatch");
		return;
	}

	try {
		auto                   b   = static_cast<document *>(bson);
		struct timeval         now = {time[0].as_integer(), time[1].as_integer()};
		bsoncxx::types::b_date nowd{
		  std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>{
		    std::chrono::milliseconds{now.tv_sec * 1000 + now.tv_usec / 1000}}};
		b->append(kvp(field_name, nowd));
	} catch (bsoncxx::exception &e) {
		logger_->log_error("MongoDB",
		                   "Failed to append time value to field %s: %s",
		                   field_name.c_str(),
		                   e.what());
	}
}

void
LLSFRefBox::clips_mongodb_insert(std::string collection, void *bson)
{
	if (!cfg_mongodb_enabled_) {
		logger_->log_warn("MongoDB", "Insert requested while MongoDB disabled");
		return;
	}

	auto b = static_cast<document *>(bson);

	try {
		database_[collection].insert_one(b->view());
	} catch (mongocxx::operation_exception &e) {
		logger_->log_warn("MongoDB", "Insert failed: %s", e.what());
	}
}


void
LLSFRefBox::mongodb_update(std::string &                  collection,
                           const bsoncxx::document::view &doc,
                           CLIPS::Value &                 query,
                           bool                           upsert)
{
	if (!cfg_mongodb_enabled_) {
		logger_->log_warn("MongoDB", "Update requested while MongoDB disabled");
		return;
	}

	try {
		bsoncxx::document::view query_view;
		if (query.type() == CLIPS::TYPE_STRING) {
			query_view = bsoncxx::from_json(query.as_string());
		} else if (query.type() == CLIPS::TYPE_EXTERNAL_ADDRESS) {
			query_view = static_cast<document *>(query.as_address())->view();
		} else {
			logger_->log_warn("MongoDB", "Invalid query, must be string or BSON document");
			return;
		}

		document update_doc{};
		update_doc.append(kvp("$set", bsoncxx::builder::concatenate(doc)));
		database_[collection].update_one(query_view,
		                                 update_doc.view(),
		                                 mongocxx::options::update().upsert(upsert));
	} catch (bsoncxx::exception &e) {
		logger_->log_warn("MongoDB", "Compiling query failed: %s", e.what());
	} catch (mongocxx::operation_exception &e) {
		logger_->log_warn("MongoDB", "Insert failed: %s", e.what());
	}
}


void
LLSFRefBox::clips_mongodb_upsert(std::string collection, void *bson, CLIPS::Value query)
{
	auto doc = static_cast<document *>(bson);
	if (!doc) {
		logger_->log_warn("MongoDB", "Invalid BSON Obj Builder passed");
		return;
	}
	mongodb_update(collection, doc->view(), query, true);
}

void
LLSFRefBox::clips_mongodb_update(std::string collection, void *bson, CLIPS::Value query)
{
	auto doc = static_cast<document *>(bson);
	if (!doc) {
		logger_->log_warn("MongoDB", "Invalid BSON Obj Builder passed");
		return;
	}

	document update_doc{};
	update_doc.append(kvp("$set", doc->view()));

	mongodb_update(collection, update_doc.view(), query, false);
}

void
LLSFRefBox::clips_mongodb_replace(std::string collection, void *bson, CLIPS::Value query)
{
	auto doc = static_cast<document *>(bson);
	if (!doc)
		logger_->log_warn("MongoDB", "Invalid BSON Obj Builder passed");
	mongodb_update(collection, doc->view(), query, false);
}

CLIPS::Value
LLSFRefBox::clips_mongodb_query_sort(std::string collection, void *bson, void *bson_sort)
{
  if (! cfg_mongodb_enabled_) {
    logger_->log_warn("MongoDB", "Query requested while MongoDB disabled");
    return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
  }

	auto doc = static_cast<document *>(bson);

	mongocxx::options::find opts{};
	if (bson_sort) {
		opts.sort(static_cast<document *>(bson_sort)->view());
	}
	try {
		auto c = std::make_unique<mongocxx::cursor>(database_[collection].find(doc->view(), opts));
		return CLIPS::Value(new std::unique_ptr<mongocxx::cursor>(std::move(c)),
		                    CLIPS::TYPE_EXTERNAL_ADDRESS);

	} catch (mongocxx::operation_exception &e) {
		logger_->log_warn("MongoDB", "Query failed: %s", e.what());
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}
}

CLIPS::Value
LLSFRefBox::clips_mongodb_query(std::string collection, void *bson)
{
	return clips_mongodb_query_sort(collection, bson, NULL);
}

/*
void
LLSFRefBox::clips_mongodb_cursor_destroy(void *cursor)
{
	auto c = static_cast<std::unique_ptr<mongocxx::cursor> *>(cursor);

	if (! c || ! c->get()) {
		logger_->log_error("MongoDB", "mongodb-cursor-destroy: got invalid cursor");
		return;
	}

	delete c;
}

CLIPS::Value
LLSFRefBox::clips_mongodb_cursor_more(void *cursor)
{
	auto c = static_cast<std::unique_ptr<mongocxx::cursor> *>(cursor);

	if (! c || ! c->get()) {
		logger_->log_error("MongoDB", "mongodb-cursor-more: got invalid cursor");
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

	return CLIPS::Value((*c)->more() ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

CLIPS::Value
LLSFRefBox::clips_mongodb_cursor_next(void *cursor)
{
	auto c = static_cast<std::unique_ptr<mongocxx::cursor> *>(cursor);

	if (! c || ! c->get()) {
		logger_->log_error("MongoDB", "mongodb-cursor-next: got invalid cursor");
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

  auto doc = new document();
  doc->append(bsoncxx::builder::concatenate((*c)->next());
	return CLIPS::Value(doc);
}
*/


CLIPS::Values
LLSFRefBox::clips_bson_field_names(void *bson)
{
	auto doc = static_cast<bsoncxx::document::value *>(bson);

	if (!doc) {
		logger_->log_error("MongoDB", "mongodb-bson-field-names: invalid object");
		CLIPS::Values rv;
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}

	auto          doc_view = doc->view();
	CLIPS::Values rv;
	std::for_each(doc_view.begin(), doc_view.end(), [&](auto &element) {
		rv.push_back(std::string(element.key()).c_str());
	});
	return rv;
}


CLIPS::Value
LLSFRefBox::clips_bson_get(void *bson, std::string field_name)
{
	auto doc = static_cast<bsoncxx::document::value *>(bson);

	if (!doc) {
		logger_->log_error("MongoDB", "mongodb-bson-get: invalid object");
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

	auto element = doc->view().find(field_name);
	if (element == doc->view().end()) {
		logger_->log_error("MongoDB", "mongodb-bson-get: has no field %s", field_name.c_str());
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

	switch (element->type()) {
	case bsoncxx::type::k_double: return CLIPS::Value(element->get_double());
	case bsoncxx::type::k_utf8: return CLIPS::Value(element->get_utf8().value.to_string());
	case bsoncxx::type::k_bool:
		return CLIPS::Value(element->get_bool() ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
	case bsoncxx::type::k_int32: return CLIPS::Value(element->get_int32());
	case bsoncxx::type::k_int64: return CLIPS::Value(element->get_int64());
	case bsoncxx::type::k_document: {
		auto obj_doc = new document();
		obj_doc->append(bsoncxx::builder::concatenate(element->get_document().view()));
		return CLIPS::Value(obj_doc);
	}
	default: return CLIPS::Value("INVALID_VALUE_TYPE", CLIPS::TYPE_SYMBOL);
	}
}


CLIPS::Values
LLSFRefBox::clips_bson_get_array(void *bson, std::string field_name)
{
	auto doc = static_cast<bsoncxx::document::value *>(bson);

	CLIPS::Values rv;

	if (!doc) {
		logger_->log_error("MongoDB", "mongodb-bson-get-array: invalid object");
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}

	auto element = doc->view().find(field_name);
	if (element == doc->view().end()) {
		logger_->log_error("MongoDB", "mongodb-bson-get-array: has no field %s", field_name.c_str());
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}

	if (element->type() != bsoncxx::type::k_array) {
		logger_->log_error("MongoDB",
		                   "mongodb-bson-get-array: field %s is not an array",
		                   field_name.c_str());
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}

	bsoncxx::array::view array_view = element->get_array();

	for (auto element = array_view.begin(); element != array_view.end(); element++) {
		switch (element->type()) {
		case bsoncxx::type::k_double: rv.push_back(CLIPS::Value(element->get_double()));
		case bsoncxx::type::k_utf8: rv.push_back(CLIPS::Value(element->get_utf8().value.to_string()));
		case bsoncxx::type::k_bool:
			CLIPS::Value(element->get_bool() ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
		case bsoncxx::type::k_int32: rv.push_back(CLIPS::Value(element->get_int32()));
		case bsoncxx::type::k_int64: rv.push_back(CLIPS::Value(element->get_int64()));
		case bsoncxx::type::k_document: {
			auto obj_doc = new document();
			obj_doc->append(bsoncxx::builder::concatenate(element->get_document().view()));
			rv.push_back(CLIPS::Value(obj_doc));
		}
		default:
			rv.clear();
			rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
			return rv;
		}
	};
	return rv;

}


CLIPS::Values
LLSFRefBox::clips_bson_get_time(void *bson, std::string field_name)
{
	auto doc = static_cast<bsoncxx::document::value *>(bson);

	CLIPS::Values rv;

	if (!doc) {
		logger_->log_error("MongoDB", "mongodb-bson-get-time: invalid object");
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}

	auto element = doc->view().find(field_name);
	if (element == doc->view().end()) {
		logger_->log_error("MongoDB", "mongodb-bson-get-time: has no field %s", field_name.c_str());
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}

	int64_t ts = 0;
	if (element->type() == bsoncxx::type::k_date) {
		bsoncxx::types::b_date d = element->get_date();
		ts                       = d.to_int64();
	} else if (element->type() == bsoncxx::type::k_timestamp) {
		bsoncxx::types::b_timestamp t = element->get_timestamp();
		ts                            = (int64_t)t.timestamp * 1000;
	} else {
		logger_->log_error("MongoDB", "mongodb-bson-get-time: field %s is not a time",
		                   field_name.c_str());
		rv.push_back(CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL));
		return rv;
	}


	rv.resize(2);
	rv[0] = CLIPS::Value((long long int)(ts / 1000));
	rv[1] = CLIPS::Value((ts - (rv[0].as_integer() * 1000)) * 1000);
	return rv;
}

#endif


void
LLSFRefBox::sps_read_rfids()
{
  if (! sps_)  return;

  //std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
  fawkes::MutexLocker lock(&clips_mutex_);


  try {
    std::vector<uint32_t> puck_ids = sps_->read_rfids();
    for (unsigned int i = 0; i < puck_ids.size(); ++i) {
      const char *machine_name = to_string(i, cfg_machine_assignment_);
      if (puck_ids[i] == SPSComm::NO_PUCK) {
        clips_->assert_fact_f("(rfid-input (machine %s) (has-puck FALSE))",
			      machine_name);
      } else {
        clips_->assert_fact_f("(rfid-input (machine %s) (has-puck TRUE) (id %u))",
			      machine_name, puck_ids[i]);
      }
    }
  } catch (fawkes::Exception &e) {
    logger_->log_warn("RefBox", "Failed to read RFIDs");
    logger_->log_warn("RefBox", e);
    try {
      sps_->try_reconnect();
      logger_->log_info("RefBox", "Successfully reconnected");
    } catch (fawkes::Exception &e) {
      logger_->log_error("RefBox", "Failed to reconnect");
      logger_->log_error("RefBox", e);
    }
  }
}


/** Start the timer for another run. */
void
LLSFRefBox::start_timer()
{
  timer_last_ = boost::posix_time::microsec_clock::local_time();
  timer_.expires_from_now(boost::posix_time::milliseconds(cfg_timer_interval_));
  timer_.async_wait(boost::bind(&LLSFRefBox::handle_timer, this,
				boost::asio::placeholders::error));
}

/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBox::handle_timer(const boost::system::error_code& error)
{
  if (! error) {
    /*
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    long ms = (now - timer_last_).total_milliseconds();
    timer_last_ = now;
    */

    //sps_read_rfids();
    if (mps_)  mps_->process();

    {
      //std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
      fawkes::MutexLocker lock(&clips_mutex_);

      if (mps_) {
	std::map<std::string, std::string> machine_states = mps_->get_states();
	for (const auto &ms : machine_states) {
	  //printf("Asserting (machine-mps-state (name %s) (state %s) (num-bases %u))\n",
	  //       ms.first.c_str(), ms.second.c_str(), 0);
          std::string type = ms.first.substr(2, 2);
          unsigned int num_bases = 0;
          if (type == "RS") {
            MPSPickPlace2 *station;
            station = mps_->get_station(ms.first, station);
            if (station)  num_bases = station->getCountSlide();
          }
	  clips_->assert_fact_f("(machine-mps-state (name %s) (state %s) (num-bases %u))",
				ms.first.c_str(), ms.second.c_str(), num_bases);
	}
      }

      clips_->assert_fact("(time (now))");
      clips_->refresh_agenda();
      clips_->run();
    }

    timer_.expires_at(timer_.expires_at()
		      + boost::posix_time::milliseconds(cfg_timer_interval_));
    timer_.async_wait(boost::bind(&LLSFRefBox::handle_timer, this,
				  boost::asio::placeholders::error));
  }
}


/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBox::handle_signal(const boost::system::error_code& error, int signum)
{
  timer_.cancel();
  io_service_.stop();
}



/** Run the application.
 * @return return code, 0 if no error, error code otherwise
 */
int
LLSFRefBox::run()
{
#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBox::handle_signal, this,
				 boost::asio::placeholders::error,
				 boost::asio::placeholders::signal_number));
#else
  g_refbox = this;
  signal(SIGINT, llsfrb::handle_signal);
#endif

  start_timer();
  io_service_.run();
  return 0;
}

} // end of namespace llsfrb
