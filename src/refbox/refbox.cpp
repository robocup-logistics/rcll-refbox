
/***************************************************************************
 *  refbox.cpp - LLSF RefBox main program
 *
 *  Created: Thu Feb 07 11:04:17 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
 *             2019  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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
#include "msgs/ProductColor.pb.h"
#include "rest-api/clips-rest-api/clips-rest-api.h"

#include <config/yaml.h>
#include <core/threading/mutex.h>
#include <core/version.h>
#include <logging/console.h>
#include <logging/file.h>
#include <logging/multi.h>
#include <logging/network.h>
#include <mps_comm/machine_factory.h>
#include <mps_comm/stations.h>
#include <mps_placing_clips/mps_placing_clips.h>
#include <protobuf_clips/communicator.h>
#include <protobuf_comm/peer.h>
#include <rest-api/webview_server.h>
#include <utils/system/argparser.h>
#include <webview/rest_api_manager.h>

#ifndef __has_include
static_assert(false, "__has_include not supported");
#else
#	if __cplusplus >= 201703L && __has_include(<filesystem>)
#		include <filesystem>
namespace fs = std::filesystem;
#	elif __has_include(<experimental/filesystem>)
#		include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#	elif __has_include(<boost/filesystem.hpp>)
#		include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#	endif
#endif

#ifdef HAVE_WEBSOCKETS
#	include <websocket/backend.h>
#	include <logging/websocket.h>
#endif

#include <boost/bind/bind.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <sstream>

#if __GNUC__ && __GNUC__ < 8
#	include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;
#else
#	include <filesystem>
namespace stdfs = std::filesystem;
#endif

namespace ph = boost::placeholders;

#include <memory>
#include <unordered_map>
#if BOOST_ASIO_VERSION < 100601
#	include <csignal>
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

#include <netcomm/utils/resolver.h>
#ifdef HAVE_AVAHI
#	include <netcomm/dns-sd/avahi_thread.h>
#	include <netcomm/service_discovery/service.h>
#else
#	include <netcomm/service_discovery/dummy_service_browser.h>
#	include <netcomm/service_discovery/dummy_service_publisher.h>
#endif

#include <memory>
#include <string>

using namespace protobuf_comm;
using namespace protobuf_clips;
using namespace llsf_utils;
using namespace fawkes;
using namespace llsfrb::mps_comm;

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
static void
handle_signal(int signum)
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
	read_config(argc, argv);

	pb_comm_ = NULL;

	cfg_clips_dir_ = std::string(SHAREDIR) + "/games/rcll/";

	cfg_timer_interval_ = config_->get_uint("/llsfrb/clips/timer-interval");

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
	try {
		std::string logfile = config_->get_string("/llsfrb/log/general");
		logger_->add_logger(new FileLogger(logfile.c_str(), log_level_));
	} catch (fawkes::Exception &e) {
	} // ignored, use default
	clips_ = std::make_unique<CLIPS::Environment>();
	setup_clips();

	std::stringstream refbox_call;
	for (int i = 0; i < argc; ++i)
		refbox_call << " " << argv[i];
	logger_->log_info("RefBox", "%s", refbox_call.str().c_str());

	setup_protobuf_comm();

#ifdef HAVE_WEBSOCKETS
	//launch websocket backend and add websocket logger
	backend_ = new websocket::Backend(logger_.get(), clips_.get(), clips_mutex_);
	backend_->start(config_->get_uint("/llsfrb/websocket/port"),
	                config_->get_bool("/llsfrb/websocket/ws-mode"),
	                config_->get_bool("/llsfrb/websocket/allow-control-all"));
	logger_->add_logger(new WebsocketLogger(backend_->get_data(), log_level_));
#endif
	mps_placing_generator_ = std::shared_ptr<mps_placing_clips::MPSPlacingGenerator>(
	  new mps_placing_clips::MPSPlacingGenerator(clips_.get(), clips_mutex_));

	logger_->add_logger(new NetworkLogger(pb_comm_->server(), log_level_));

#ifdef HAVE_WEBSOCKETS
	setup_clips_websocket();
#endif

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
		clips_logger_->add_logger(new MongoDBLogLogger(cfg_mongodb_hostport_, mdb_text_log));

		clips_logger_->add_logger(new MongoDBLogLogger(cfg_mongodb_hostport_, mdb_clips_log));

		mongodb_protobuf_ = std::make_unique<MongoDBLogProtobuf>(cfg_mongodb_hostport_, mdb_protobuf);

		client_   = mongocxx::client{mongocxx::uri{"mongodb://" + cfg_mongodb_hostport_}};
		database_ = client_["rcll"];

		setup_clips_mongodb();

		pb_comm_->server()->signal_received().connect(
		  boost::bind(&LLSFRefBox::handle_server_client_msg, this, ph::_1, ph::_2, ph::_3, ph::_4));
		pb_comm_->server()->signal_receive_failed().connect(
		  boost::bind(&LLSFRefBox::handle_server_client_fail, this, ph::_1, ph::_2, ph::_3, ph::_4));

		pb_comm_->signal_server_sent().connect(
		  boost::bind(&LLSFRefBox::handle_server_sent_msg, this, ph::_1, ph::_2));
		pb_comm_->signal_client_sent().connect(
		  boost::bind(&LLSFRefBox::handle_client_sent_msg, this, ph::_1, ph::_2, ph::_3));
		pb_comm_->signal_peer_sent().connect(
		  boost::bind(&LLSFRefBox::handle_peer_sent_msg, this, ph::_2));
	}
#endif

	start_clips();

#ifdef HAVE_MONGODB
	// we can do this only after CLIPS was started as it initiates the private peers
	if (cfg_mongodb_enabled_) {
		const std::map<long int, protobuf_comm::ProtobufBroadcastPeer *> &peers = pb_comm_->peers();
		for (auto p : peers) {
			p.second->signal_received().connect(
			  boost::bind(&LLSFRefBox::handle_peer_msg, this, ph::_1, ph::_2, ph::_3, ph::_4));
		}
	}
#endif

	std::shared_ptr<fawkes::ServicePublisher>    service_publisher;
	std::shared_ptr<fawkes::ServiceBrowser>      service_browser;
	std::unique_ptr<fawkes::NetworkNameResolver> nnresolver;

#ifdef HAVE_AVAHI
	unsigned int refbox_port = config_->get_uint("/llsfrb/comm/server-port");
	avahi_thread_            = std::make_shared<AvahiThread>();
	service_publisher        = avahi_thread_;
	service_browser          = avahi_thread_;
	avahi_thread_->start();
	nnresolver      = std::make_unique<fawkes::NetworkNameResolver>(avahi_thread_.get());
	refbox_service_ = std::make_unique<fawkes::NetworkService>(nnresolver.get(),
	                                                           "RefBox on %h",
	                                                           "_refbox._tcp",
	                                                           refbox_port);
	avahi_thread_->publish_service(refbox_service_.get());
#else
	service_publisher = std::make_unique<fawkes::DummyServicePublisher>();
	service_browser   = std::make_unique<fawkes::DummyServiceBrowser>();
	nnresolver        = std::make_unique<fawkes::NetworkNameResolver>();
#endif

	try {
		clips_rest_api_ = std::make_unique<ClipsRestApi>(clips_.get(), clips_mutex_, logger_.get());

		rest_api_manager_ = std::make_shared<WebviewRestApiManager>();
		rest_api_manager_->register_api(clips_rest_api_.get());

		rest_api_thread_ = std::make_unique<llsfrb::WebviewServer>(false,
		                                                           rest_api_manager_,
		                                                           std::move(nnresolver),
		                                                           service_publisher,
		                                                           service_browser,
		                                                           config_.get(),
		                                                           logger_.get());
		rest_api_thread_->start();

	} catch (Exception &e) {
		logger_->log_info("RefBox", "Could not start RESTapi");
		logger_->log_error("Exception: ", e.what());
	}

	// gather all yaml files that one could choose from
	std::vector<std::string> all_yaml_files;
	for (const auto &p : fs::recursive_directory_iterator(CONFDIR)) {
		if (fs::is_regular_file(p.path()) && p.path().extension() == ".yaml") {
			all_yaml_files.push_back(p.path().string());
			std::string relative_path = "";
			try {
				relative_path = p.path().parent_path().string().substr(std::strlen(CONFDIR) + 1);
			} catch (const std::out_of_range &) {
			}
			std::string filename = p.path().stem().string();
			clips_->assert_fact_f("(cfg-preset (category \"%s\") (preset \"%s\"))",
			                      relative_path.c_str(),
			                      filename.c_str());
		}
	}
}

/** Destructor. */
LLSFRefBox::~LLSFRefBox()
{
	timer_.cancel();

	rest_api_thread_->cancel();
	rest_api_thread_->join();

	rest_api_manager_->unregister_api(clips_rest_api_.get());
#ifdef HAVE_AVAHI
	avahi_thread_->cancel();
	avahi_thread_->join();
#endif

	//std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
	{
		fawkes::MutexLocker lock(&clips_mutex_);
		clips_->assert_fact("(finalize)");
		clips_->refresh_agenda();
		clips_->run();

		finalize_clips_logger(clips_->cobj());
	}

	mps_placing_generator_.reset();

	// Delete all global objects allocated by libprotobuf
}

/** Read yaml configurations based on given command line options.
 * @param argc number of arguments passed
 * @param argv array of arguments
 */
void
LLSFRefBox::read_config(int argc, char **argv)
{
	// key: cfg option, value: path to file
	std::map<std::string, std::string> cfg_files_to_include;
	std::vector<std::string>           gen_options_str;
	// collect folders and default files
	for (auto &p : fs::directory_iterator(CONFDIR)) {
		if (fs::is_directory(p.status())) {
			std::string cfg_opt = "cfg-" + p.path().filename().string();
			std::string default_cfg =
			  p.path().string() + "/default_" + p.path().filename().string() + ".yaml";
			if (fs::exists(fs::path(default_cfg))) {
				cfg_files_to_include[cfg_opt] = default_cfg;
				gen_options_str.push_back(cfg_opt);
			}
		}
	}
	// Populate generated options, stuck to raw arrays as the char arrays within
	// the option struct can be invalidated through smart containers
	option generated_options[cfg_files_to_include.size()];
	for (size_t i = 0; i < gen_options_str.size(); i++) {
		generated_options[i].name    = gen_options_str[i].c_str();
		generated_options[i].has_arg = 1;
		generated_options[i].flag    = 0;
		generated_options[i].val     = 0;
	}
	// Add custom command line options here
	std::vector<option> static_options = {{"no-default-cfg", 0, 0, 0},
	                                      {"cfg-custom", 1, 0, 0},
	                                      {"dump-cfg", 0, 0, 0},
	                                      {0, 0, 0, 0}}; // null terminate options
	option              options[cfg_files_to_include.size() + static_options.size()];
	// Prepare ArgumentParser
	for (size_t i = 0; i < gen_options_str.size(); i++) {
		options[i] = generated_options[i];
	}

	for (size_t i = 0; i < static_options.size(); i++) {
		options[gen_options_str.size() + i] = static_options[i];
	}

	// Process provided options
	ArgumentParser argp(argc, argv, "h", options);
	if (argp.has_arg("h")) {
		std::string help_message = "1. Config overrides:\n\n";
		size_t      longest_opt  = std::max_element(gen_options_str.begin(),
                                          gen_options_str.end(),
                                          [](const std::string &lhs, const std::string &rhs) {
                                            return lhs.size() < rhs.size();
                                          })
		                       ->size();
		for (const auto &cfg_option : cfg_files_to_include) {
			std::string padding(longest_opt - cfg_option.first.size(), ' ');
			;
			help_message += "  --" + cfg_option.first + " <yaml-file> " + padding
			                + ": load <yaml-file> instead of " + cfg_option.second + "\n";
		}
		help_message += "\n2. Other options:\n\n";
		help_message += "  --no-default-cfg             : do not load any specific default configs\n";
		help_message +=
		  "  --cfg-custom <yaml-file>     : load an additional <yaml-file> (loaded last)\n";
		help_message += "  --dump-cfg <yaml-file>       : write the configuration file (required to "
		                "use some of the companion tools)\n";
		printf("--- RefBox customization options ---\n%s", help_message.c_str());
		exit(1);
	}
	if (argp.has_arg("no-default-cfg")) {
		cfg_files_to_include.clear();
	}
	for (const auto &opt : gen_options_str) {
		if (argp.arg(opt.c_str())) {
			std::string opt_arg(argp.arg(opt.c_str()));
			cfg_files_to_include[opt] = opt_arg;
		}
	}
	bool          dump_cfg = argp.has_arg("dump-cfg");
	std::ofstream generated_cfg_file;

	if (dump_cfg) {
		// generate the configuration file
		generated_cfg_file.open(std::string(CONFDIR) + "/config_generated.yaml");
		generated_cfg_file <<
		  R"delimiter(%YAML 1.2
%TAG ! tag:fawkesrobotics.org,cfg/
---
# Generated Configuration meta information document, do not modify this!
include:
)delimiter";
	}
	config_ = std::make_shared<YamlConfiguration>(CONFDIR);
	for (const auto &std_val : cfg_files_to_include) {
		if (dump_cfg) {
			generated_cfg_file << " - " << std_val.second.c_str() << "\n";
		}
		config_->load(std_val.second.c_str());
	}
	if (argp.arg("cfg-custom")) {
		if (dump_cfg) {
			std::string opt_arg(argp.arg("cfg-custom"));
		}
		config_->load(argp.arg("cfg-custom"));
	}
	if (dump_cfg) {
		generated_cfg_file << "---\n";
		generated_cfg_file.close();
	}
	std::shared_ptr<Configuration::ValueIterator> v(config_->search("llsfrb"));
}

void
LLSFRefBox::setup_protobuf_comm()
{
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
				if ((pos = proto_dirs[i].find("@SHAREDIR@")) != std::string::npos) {
					proto_dirs[i].replace(pos, 10, SHAREDIR);
				}

				if (proto_dirs[i][proto_dirs.size() - 1] != '/') {
					proto_dirs[i] += "/";
				}
				//logger_->log_warn("RefBox", "DIR: %s", proto_dirs[i].c_str());
			}
		}
	} catch (fawkes::Exception &e) {
	} // ignore, use default

	if (proto_dirs.empty()) {
		pb_comm_ = std::make_unique<ClipsProtobufCommunicator>(clips_.get(), clips_mutex_);
	} else {
		pb_comm_ = std::make_unique<ClipsProtobufCommunicator>(clips_.get(), clips_mutex_, proto_dirs);
	}

	pb_comm_->enable_server(config_->get_uint("/llsfrb/comm/server-port"));

	MessageRegister &mr_server = pb_comm_->message_register();
	if (!mr_server.load_failures().empty()) {
		MessageRegister::LoadFailMap::const_iterator e      = mr_server.load_failures().begin();
		std::string                                  errstr = e->first + " (" + e->second + ")";
		for (++e; e != mr_server.load_failures().end(); ++e) {
			errstr += std::string(", ") + e->first + " (" + e->second + ")";
		}
		logger_->log_warn("RefBox", "Failed to load some message types: %s", errstr.c_str());
	}
}

void
LLSFRefBox::setup_clips()
{
	fawkes::MutexLocker lock(&clips_mutex_);

	logger_->log_info("RefBox", "Creating CLIPS environment");
	clips_logger_ = std::make_unique<MultiLogger>();
	clips_logger_->add_logger(new ConsoleLogger(log_level_));
	try {
		std::string logfile = config_->get_string("/llsfrb/log/clips");
		clips_logger_->add_logger(new FileLogger(logfile.c_str(), Logger::LL_DEBUG));
	} catch (fawkes::Exception &e) {
	} // ignored, use default
	if (config_->get_bool_or_default("/llsfrb/clips/debug", false)) {
		clips_->evaluate("(watch rules)");
		clips_->evaluate("(watch facts)");
	}

	init_clips_logger(clips_->cobj(), logger_.get(), clips_logger_.get());

	std::string defglobal_ver =
	  boost::str(boost::format("(defglobal\n"
	                           "  ?*VERSION-MAJOR* = %u\n"
	                           "  ?*VERSION-MINOR* = %u\n"
	                           "  ?*VERSION-MICRO* = %u\n"
	                           ")")
	             % FAWKES_VERSION_MAJOR % FAWKES_VERSION_MINOR % FAWKES_VERSION_MICRO);

	clips_->build(defglobal_ver);

	clips_->add_function("get-clips-dirs",
	                     sigc::slot<CLIPS::Values>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_get_clips_dirs)));
	clips_->add_function("now",
	                     sigc::slot<CLIPS::Values>(sigc::mem_fun(*this, &LLSFRefBox::clips_now)));
	clips_->add_function("load-config",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_load_config)));
	clips_->add_function("config-path-exists",
	                     sigc::slot<CLIPS::Value, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_path_exists)));
	clips_->add_function("config-get-bool",
	                     sigc::slot<CLIPS::Value, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_get_bool)));
	clips_->add_function("config-get-int",
	                     sigc::slot<CLIPS::Value, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_get_int)));
	clips_->add_function("print-fact-list",
	                     sigc::slot<void, CLIPS::Values, CLIPS::Values>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_print_fact_list)));

	clips_->add_function("mps-move-conveyor",
	                     sigc::slot<void, std::string, std::string, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_move_conveyor)));
	clips_->add_function("mps-cs-retrieve-cap",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_cs_retrieve_cap)));
	clips_->add_function("mps-cs-mount-cap",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_cs_mount_cap)));
	clips_->add_function("mps-bs-dispense",
	                     sigc::slot<void, std::string, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_bs_dispense)));

	clips_->add_function("mps-set-light",
	                     sigc::slot<void, std::string, std::string, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_set_light)));
	clips_->add_function("mps-set-lights",
	                     sigc::slot<void, std::string, std::string, std::string, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_set_lights)));
	clips_->add_function("mps-reset-lights",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_reset_lights)));
	clips_->add_function("mps-ds-process",
	                     sigc::slot<void, std::string, int>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_ds_process)));
	clips_->add_function("mps-rs-mount-ring",
	                     sigc::slot<void, std::string, int, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_rs_mount_ring)));
	clips_->add_function("mps-reset",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_reset)));
	clips_->add_function("mps-reset-base-counter",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_reset_base_counter)));
	clips_->add_function("mps-deliver",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_deliver)));
	clips_->add_function("mps-ss-retrieve",
	                     sigc::slot<void, std::string, int, int>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_ss_retrieve)));
	clips_->add_function("mps-ss-store",
	                     sigc::slot<void, std::string, int, int>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_ss_store)));
	clips_->add_function("mps-ss-relocate",
	                     sigc::slot<void, std::string, int, int, int, int>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mps_ss_relocate)));
	clips_->add_function("config-update-float",
	                     sigc::slot<void, std::string, float>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_update_float)));
	clips_->add_function("config-update-uint",
	                     sigc::slot<void, std::string, int>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_update_uint)));

	clips_->add_function("config-update-int",
	                     sigc::slot<void, std::string, int>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_update_int)));

	clips_->add_function("config-update-bool",
	                     sigc::slot<void, std::string, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_update_bool)));

	clips_->add_function("config-update-string",
	                     sigc::slot<void, std::string, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_config_update_string)));
	clips_->add_function("reconfigure-machine",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_add_machine)));

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
	std::queue<int>                                    to_erase;
	std::map<long int, CLIPS::Fact::pointer>::iterator f;

	for (f = clips_msg_facts_.begin(); f != clips_msg_facts_.end(); ++f) {
		if (f->second->refcount() == 1) {
			//logger_->log_info("RefBox", "Fact %li can be erased", f->second->index());
			to_erase.push(f->first);
		}
	}
	while (!to_erase.empty()) {
		long int              index = to_erase.front();
		CLIPS::Fact::pointer &f     = clips_msg_facts_[index];
		CLIPS::Value          v     = f->slot_value("ptr")[0];
		void                 *ptr   = v.as_address();
		delete static_cast<std::shared_ptr<google::protobuf::Message> *>(ptr);
		clips_msg_facts_.erase(index);
		to_erase.pop();
	}
}

CLIPS::Values
LLSFRefBox::clips_now()
{
	CLIPS::Values  rv;
	struct timeval tv;
	gettimeofday(&tv, 0);
	rv.push_back(tv.tv_sec);
	rv.push_back(tv.tv_usec);
	return rv;
}

/** Convert a clips value into a string representation
 * @param v Value to convert
 * @return v represented as std::string
 */
std::string
LLSFRefBox::clips_value_to_string(const CLIPS::Value &v)
{
	switch (v.type()) {
	case CLIPS::Type::TYPE_FLOAT: return std::to_string(v.as_float());
	case CLIPS::Type::TYPE_STRING:
	case CLIPS::Type::TYPE_SYMBOL: return v.as_string();
	case CLIPS::Type::TYPE_INTEGER: return std::to_string(v.as_integer());
	default: return "";
	}
}

CLIPS::Values
LLSFRefBox::clips_get_clips_dirs()
{
	CLIPS::Values rv;
	rv.push_back(cfg_clips_dir_);
	return rv;
}

/** Print a list of facts as a formatted table
 * @param facts A multifield of fact indices, which all belong to the same
 *              template
 * @param fields A multifield of field names that belong to the template of
 *               the given fact. If an empty multifield is supplied, then
 *               all fields of the template are considered.
 *
 * Prints out a table, where each row consists of field values (from fields)
 * of a fact from the given fact list.
 */
void
LLSFRefBox::clips_print_fact_list(CLIPS::Values facts, CLIPS::Values fields)
{
	if (facts.size() == 0) {
		return;
	}
	MutexLocker           lock(&clips_mutex_);
	std::vector<long int> fact_indices;
	try {
		std::transform(facts.begin(),
		               facts.end(),
		               std::back_inserter(fact_indices),
		               [this](const CLIPS::Value &s) -> long int { return s.as_integer(); });
	} catch (Exception &e) {
		logger_->log_error("print-fact-list", "Expected fact-index %s");
	}
	std::map<long int, CLIPS::Fact::pointer> retrieved_facts;
	CLIPS::Fact::pointer                     fact = clips_->get_facts();

	CLIPS::Template::pointer fact_template;
	while (fact) {
		if (std::find(fact_indices.begin(), fact_indices.end(), fact->index()) != fact_indices.end()) {
			retrieved_facts[fact->index()] = fact;
			if (!fact_template) {
				fact_template = fact->get_template();
			} else {
				if (fact_template->name() != fact->get_template()->name()) {
					logger_->log_error("print-fact-list", "Expected facts from exactly one template");
					return;
				}
			}
		}
		fact = fact->next();
	}

	std::map<std::string, size_t> slot_value_length;
	std::vector<std::string>      slot_names;

	for (const auto &field : fields) {
		slot_names.push_back(clips_value_to_string(field));
	}
	if (slot_names.size() == 0) {
		slot_names = fact_template->slot_names();
	}

	for (auto slot_name : slot_names) {
		slot_value_length.emplace(slot_name, 0);
	}

	for (const auto &f : fact_indices) {
		auto elem = retrieved_facts.find(f);
		if (elem != retrieved_facts.end()) {
			for (const auto &slot_name : slot_names) {
				size_t slot_val_length = 0;
				for (const auto &val : elem->second->slot_value(slot_name)) {
					slot_val_length += clips_value_to_string(val).size() + 1;
				}
				slot_value_length[slot_name] =
				  std::max({slot_value_length[slot_name], slot_val_length, slot_name.size()});
			}
		} else {
			logger_->log_error("print-fact-list", "Expected fact-index %s");
			return;
		}
	}

	std::stringstream table_header;
	table_header << " | ";
	std::stringstream table_sep;
	table_sep << "---";
	for (auto &slot_name : slot_names) {
		table_header << std::left << std::setw(slot_value_length[slot_name]) << std::setfill(' ')
		             << slot_name << " | ";
		table_sep << std::left << std::setw(slot_value_length[slot_name]) << std::setfill('-') << "-"
		          << "---";
	}
	clips_logger_->log_info("C", table_header.str().c_str());
	clips_logger_->log_info("C", table_sep.str().c_str());
	for (auto &fact_id : fact_indices) {
		std::stringstream row;
		row << " | ";
		for (auto &slot_name : slot_names) {
			std::string slot_str;
			for (const auto &val : retrieved_facts[fact_id]->slot_value(slot_name)) {
				slot_str += clips_value_to_string(val) + " ";
			}
			row << std::left << std::setw(slot_value_length[slot_name]) << std::setfill(' ') << slot_str
			    << " | ";
		}
		clips_logger_->log_info("C", row.str().c_str());
	}
}

void
LLSFRefBox::clips_load_config(std::string cfg_prefix)
{
	std::shared_ptr<Configuration::ValueIterator> v(config_->search(cfg_prefix.c_str()));
	while (v->next()) {
		clips_assert_confval(v);
	}
}

void
LLSFRefBox::clips_assert_confval(std::shared_ptr<Configuration::ValueIterator> v)
{
	std::string type  = "";
	std::string value = v->get_as_string();

	if (v->is_uint())
		type = "UINT";
	else if (v->is_int())
		type = "INT";
	else if (v->is_float())
		type = "FLOAT";
	else if (v->is_bool()) {
		type  = "BOOL";
		value = v->get_bool() ? "TRUE" : "FALSE";
	} else if (v->is_string()) {
		type = "STRING";
		if (!v->is_list()) {
			value = std::string("\"") + value + "\"";
		}
	} else {
		logger_->log_warn("RefBox", "Config value at '%s' of unknown type '%s'", v->path(), v->type());
	}
	CLIPS::Fact::pointer fact = clips_->get_facts();
	while (fact) {
		if (fact->get_template()->name() == "confval") {
			try {
				if (fact->slot_value("path")[0].as_string() == v->path()) {
					fact->retract();
					break;
				}
			} catch (Exception &e) {
				logger_->log_error("RefBox", "can't access path slot of confval fact");
			}
		}
		fact = fact->next();
	}

	if (v->is_list()) {
		//logger_->log_info("RefBox", "(confval (path \"%s\") (type %s) (is-list TRUE) (list-value %s))",
		//       v->path(), type.c_str(), value.c_str());
		clips_->assert_fact_f("(confval (path \"%s\") (type %s) (is-list TRUE) (list-value %s))",
		                      v->path(),
		                      type.c_str(),
		                      value.c_str());
	} else {
		//logger_->log_info("RefBox", "(confval (path \"%s\") (type %s) (value %s))",
		//       v->path(), type.c_str(), value.c_str());
		clips_->assert_fact_f("(confval (path \"%s\") (type %s) (value %s))",
		                      v->path(),
		                      type.c_str(),
		                      value.c_str());
	}
}
CLIPS::Value
LLSFRefBox::clips_config_path_exists(std::string path)
{
	return CLIPS::Value(config_->exists(path.c_str()) ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}

void
LLSFRefBox::clips_config_update_float(std::string path, float f)
{
	config_->set_float(path.c_str(), f);
}
void
LLSFRefBox::clips_config_update_uint(std::string path, int i)
{
	config_->set_uint(path.c_str(), i);
}

void
LLSFRefBox::clips_config_update_int(std::string path, int i)
{
	config_->set_int(path.c_str(), i);
}

void
LLSFRefBox::clips_config_update_bool(std::string path, std::string b)
{
	if (b == "FALSE") {
		config_->set_bool(path.c_str(), false);
	} else {
		config_->set_bool(path.c_str(), true);
	}
}

void
LLSFRefBox::clips_config_update_string(std::string path, std::string s)
{
	config_->set_string(path.c_str(), s);
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

CLIPS::Value
LLSFRefBox::clips_config_get_int(std::string path)
{
	try {
		int v = config_->get_int(path.c_str());
		return CLIPS::Value(v);
	} catch (Exception &e) {
		return CLIPS::Value(0);
	}
}

bool
LLSFRefBox::mutex_future_ready(const std::string &name)
{
	auto mf_it = mutex_futures_.find(name);
	if (mf_it != mutex_futures_.end()) {
		auto fut_status = mutex_futures_[name].wait_for(std::chrono::milliseconds(0));
		if (fut_status != std::future_status::ready) {
			return false;
		} else {
			mutex_futures_.erase(mf_it);
		}
	}
	return true;
}

void
LLSFRefBox::clips_mps_reset(std::string machine)
{
	logger_->log_info("MPS", "Resetting machine %s", machine.c_str());

	llsfrb::mps_comm::Machine *station;
	try {
		station = mps_.at(machine).get();
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	if (!mutex_future_ready(machine)) {
		return;
	}
	auto fut = std::async(std::launch::async, [station, machine] {
		station->reset();
		return true;
	});

	mutex_futures_[machine] = std::move(fut);
}

void
LLSFRefBox::clips_mps_reset_base_counter(std::string machine)
{
	// TODO implement
	logger_->log_info("MPS", "Resetting machine %s", machine.c_str());
	return;
}

void
LLSFRefBox::clips_mps_deliver(std::string machine)
{
	logger_->log_info("MPS", "Delivering on %s", machine.c_str());

	llsfrb::mps_comm::Machine *station;
	try {
		station = mps_.at(machine).get();
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	if (!mutex_future_ready(machine)) {
		return;
	}
	auto fut = std::async(std::launch::async, [this, station, machine] {
		station->conveyor_move(llsfrb::mps_comm::Machine::ConveyorDirection::FORWARD,
		                       llsfrb::mps_comm::Machine::MPSSensor::OUTPUT);
		MutexLocker lock(&clips_mutex_);
		clips_->assert_fact_f("(mps-feedback mps-deliver success %s)", machine.c_str());
		return true;
	});

	mutex_futures_[machine] = std::move(fut);
}

void
LLSFRefBox::clips_mps_bs_dispense(std::string machine, std::string color)
{
	logger_->log_info("MPS", "Dispense %s: %s", machine.c_str(), color.c_str());
	BaseStation *station;
	try {
		station = dynamic_cast<BaseStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	llsf_msgs::BaseColor color_id;
	if (color == "BASE_RED") {
		color_id = llsf_msgs::BaseColor::BASE_RED;
	} else if (color == "BASE_SILVER") {
		color_id = llsf_msgs::BaseColor::BASE_SILVER;
	} else if (color == "BASE_BLACK") {
		color_id = llsf_msgs::BaseColor::BASE_BLACK;
	} else {
		logger_->log_error("MPS", "Invalid color %s", color.c_str());
		return;
	}
	station->get_base(color_id);
}

void
LLSFRefBox::clips_mps_ds_process(std::string machine, int slide)
{
	logger_->log_info("MPS", "Processing on %s: slide %d", machine.c_str(), slide);
	DeliveryStation *station;
	try {
		station = dynamic_cast<DeliveryStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->deliver_product(slide);
}

void
LLSFRefBox::clips_mps_rs_mount_ring(std::string machine, int slide, std::string color)
{
	logger_->log_info("MPS", "Mount ring on %s: slide %d", machine.c_str(), slide);
	RingStation *station;
	try {
		station = dynamic_cast<RingStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	llsf_msgs::RingColor ring_color;
	if (color == "RING_BLUE") {
		ring_color = llsf_msgs::RING_BLUE;
	} else if (color == "RING_GREEN") {
		ring_color = llsf_msgs::RING_GREEN;
	} else if (color == "RING_ORANGE") {
		ring_color = llsf_msgs::RING_ORANGE;
	} else if (color == "RING_YELLOW") {
		ring_color = llsf_msgs::RING_YELLOW;
	} else {
		logger_->log_info("MPS", "Unexpected ring color %s", color.c_str());
		return;
	}
	station->mount_ring(slide, ring_color);
}

void
LLSFRefBox::clips_mps_move_conveyor(std::string machine,
                                    std::string goal_position,
                                    std::string conveyor_direction /*= "FORWARD"*/)
{
	llsfrb::mps_comm::Machine *station;
	try {
		station = mps_.at(machine).get();
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	llsfrb::mps_comm::Machine::MPSSensor goal;
	if (goal_position == "INPUT") {
		goal = llsfrb::mps_comm::Machine::MPSSensor::INPUT;
	} else if (goal_position == "MIDDLE") {
		goal = llsfrb::mps_comm::Machine::MPSSensor::MIDDLE;
	} else if (goal_position == "OUTPUT") {
		goal = llsfrb::mps_comm::Machine::MPSSensor::OUTPUT;
	} else {
		logger_->log_error("MPS", "Unknown conveyor position %s", goal_position.c_str());
		return;
	}
	llsfrb::mps_comm::Machine::ConveyorDirection direction;
	if (conveyor_direction == "FORWARD") {
		direction = llsfrb::mps_comm::Machine::ConveyorDirection::FORWARD;
	} else if (conveyor_direction == "BACKWARD") {
		direction = llsfrb::mps_comm::Machine::ConveyorDirection::BACKWARD;
	} else {
		logger_->log_error("MPS", "Unknown conveyor direction %s", conveyor_direction.c_str());
		return;
	}
	station->conveyor_move(direction, goal);
}

void
LLSFRefBox::clips_mps_cs_retrieve_cap(std::string machine)
{
	CapStation *station;
	try {
		station = dynamic_cast<CapStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->retrieve_cap();
}

void
LLSFRefBox::clips_mps_cs_mount_cap(std::string machine)
{
	CapStation *station;
	try {
		station = dynamic_cast<CapStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->mount_cap();
}

void
LLSFRefBox::clips_mps_ss_retrieve(std::string machine, unsigned int shelf, unsigned int slot)
{
	StorageStation *station;
	try {
		station = dynamic_cast<StorageStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->retrieve(shelf, slot);
}

void
LLSFRefBox::clips_mps_ss_store(std::string machine, unsigned int shelf, unsigned int slot)
{
	StorageStation *station;
	try {
		station = dynamic_cast<StorageStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->store(shelf, slot);
}

void
LLSFRefBox::clips_mps_ss_relocate(std::string  machine,
                                  unsigned int shelf,
                                  unsigned int slot,
                                  unsigned int target_shelf,
                                  unsigned int target_slot)
{
	StorageStation *station;
	try {
		station = dynamic_cast<StorageStation *>(mps_.at(machine).get());
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->relocate(shelf, slot, target_shelf, target_slot);
}

void
LLSFRefBox::clips_mps_set_light(std::string machine, std::string color, std::string state)
{
	//logger_->log_info("MPS", "Set light %s: %s to %s",
	//		    machine.c_str(), color.c_str(), state.c_str());

	llsfrb::mps_comm::Machine *station;
	try {
		station = mps_.at(machine).get();
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	llsf_msgs::LightColor color_id;
	if (color == "RED") {
		color_id = llsf_msgs::LightColor::RED;
	} else if (color == "YELLOW") {
		color_id = llsf_msgs::LightColor::YELLOW;
	} else if (color == "GREEN") {
		color_id = llsf_msgs::LightColor::GREEN;
	} else {
		logger_->log_error("MPS", "Invalid color %s", color.c_str());
		return;
	}

	llsf_msgs::LightState state_id;
	if (state == "ON") {
		state_id = llsf_msgs::LightState::ON;
	} else if (state == "BLINK") {
		state_id = llsf_msgs::LightState::BLINK;
	} else if (state == "OFF") {
		state_id = llsf_msgs::LightState::OFF;
	} else {
		logger_->log_error("MPS", "Invalid state %s", state.c_str());
		return;
	}

	//printf("Set light %i %i %i\n", color_id, state_id, blink_id);
	// TODO time?
	int time = 0;
	station->set_light(color_id, state_id, time);
}

void
LLSFRefBox::clips_mps_set_lights(std::string machine,
                                 std::string red_state,
                                 std::string yellow_state,
                                 std::string green_state)
{
	clips_mps_set_light(machine, "RED", red_state);
	clips_mps_set_light(machine, "YELLOW", yellow_state);
	clips_mps_set_light(machine, "GREEN", green_state);
}

void
LLSFRefBox::clips_mps_reset_lights(std::string machine)
{
	llsfrb::mps_comm::Machine *station;
	try {
		station = mps_.at(machine).get();
	} catch (std::out_of_range &e) {
		logger_->log_error("MPS", "Invalid station %s", machine.c_str());
		return;
	}
	station->reset_light();
}

#ifdef HAVE_MONGODB

/** Handle message that came from a client.
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message
 */
void
LLSFRefBox::handle_server_client_msg(ProtobufStreamServer::ClientID             client,
                                     uint16_t                                   component_id,
                                     uint16_t                                   msg_type,
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
LLSFRefBox::handle_peer_msg(boost::asio::ip::udp::endpoint            &endpoint,
                            uint16_t                                   component_id,
                            uint16_t                                   msg_type,
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
                                      uint16_t                       component_id,
                                      uint16_t                       msg_type,
                                      std::string                    msg)
{
}

void
LLSFRefBox::add_comp_type(google::protobuf::Message &m, document *doc)
{
	const google::protobuf::Descriptor     *desc     = m.GetDescriptor();
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
LLSFRefBox::handle_server_sent_msg(ProtobufStreamServer::ClientID             client,
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
LLSFRefBox::handle_client_sent_msg(std::string                                host,
                                   unsigned short int                         port,
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
	clips_->add_function("bson-builder-destroy",
	                     sigc::slot<void, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_builder_destroy)));
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
	                     sigc::slot<CLIPS::Value>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_bson_array_start)));
	clips_->add_function("bson-array-finish",
	                     sigc::slot<void, void *, std::string, void *>(
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
	clips_->add_function("mongodb-cursor-destroy",
	                     sigc::slot<void, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_cursor_destroy)));
	/*
	clips_->add_function("mongodb-cursor-more",
	                     sigc::slot<CLIPS::Value, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_cursor_more)));
  */
	clips_->add_function("mongodb-cursor-next",
	                     sigc::slot<CLIPS::Value, void *>(
	                       sigc::mem_fun(*this, &LLSFRefBox::clips_mongodb_cursor_next)));
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
LLSFRefBox::clips_bson_builder_destroy(void *bson)
{
	auto b = static_cast<bsoncxx::builder::basic::document *>(bson);
	delete b;
}

void
LLSFRefBox::clips_bson_destroy(void *bson)
{
	auto b = static_cast<bsoncxx::document::value *>(bson);
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
			if (value == "FALSE") {
				b->append(kvp(field_name, false));
				break;
			}
			if (value == "TRUE") {
				b->append(kvp(field_name, true));
				break;
			}
			if (value == "nil") {
				b->append(kvp(field_name, bsoncxx::types::b_null{}));
				break;
			}
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
LLSFRefBox::clips_bson_append_array(void *bson, std::string field_name, CLIPS::Values values)
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
LLSFRefBox::clips_bson_array_start()
{
	return CLIPS::Value(new bsoncxx::builder::basic::array{});
}

void
LLSFRefBox::clips_bson_array_finish(void *bson, std::string field_name, void *array)
{
	auto doc       = static_cast<document *>(bson);
	auto array_doc = static_cast<bsoncxx::builder::basic::array *>(array);
	doc->append(kvp(field_name, array_doc->view()));
	delete array_doc;
}

void
LLSFRefBox::clips_bson_array_append(void *array, CLIPS::Value value)
{
	auto array_doc = static_cast<bsoncxx::builder::basic::array *>(array);
	switch (value.type()) {
	case CLIPS::TYPE_FLOAT: array_doc->append(value.as_float()); break;

	case CLIPS::TYPE_INTEGER: array_doc->append(static_cast<int64_t>(value.as_integer())); break;

	case CLIPS::TYPE_SYMBOL:
	case CLIPS::TYPE_STRING:
	case CLIPS::TYPE_INSTANCE_NAME: array_doc->append(value.as_string()); break;

	case CLIPS::TYPE_EXTERNAL_ADDRESS: {
		auto subb = static_cast<document *>(value.as_address());
		array_doc->append(subb->view());
	} break;
	default:
		logger_->log_warn("MongoDB",
		                  "bson-array-append: tried to add unknown type to BSON array field");
		break;
	}
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
LLSFRefBox::mongodb_update(std::string                   &collection,
                           const bsoncxx::document::view &doc,
                           CLIPS::Value                  &query,
                           bool                           upsert)
{
	if (!cfg_mongodb_enabled_) {
		logger_->log_warn("MongoDB", "Update requested while MongoDB disabled");
		return;
	}

	try {
		document update_doc{};
		update_doc.append(kvp("$set", bsoncxx::builder::concatenate(doc)));
		if (query.type() == CLIPS::TYPE_STRING) {
			auto query_doc = bsoncxx::from_json(query.as_string());
			database_[collection].update_one(query_doc.view(),
			                                 update_doc.view(),
			                                 mongocxx::options::update().upsert(upsert));
		} else if (query.type() == CLIPS::TYPE_EXTERNAL_ADDRESS) {
			auto query_doc = static_cast<document *>(query.as_address());
			database_[collection].update_one(query_doc->view(),
			                                 update_doc.view(),
			                                 mongocxx::options::update().upsert(upsert));
		} else {
			logger_->log_warn("MongoDB", "Invalid query, must be string or BSON document");
			return;
		}
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

	mongodb_update(collection, doc->view(), query, false);
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
	if (!cfg_mongodb_enabled_) {
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

void
LLSFRefBox::clips_mongodb_cursor_destroy(void *cursor)
{
	auto c = static_cast<std::unique_ptr<mongocxx::cursor> *>(cursor);

	if (!c || !c->get()) {
		logger_->log_error("MongoDB", "mongodb-cursor-destroy: got invalid cursor");
		return;
	}

	delete c;
}

/*
CLIPS::Value
LLSFRefBox::clips_mongodb_cursor_more(void *cursor)
{
	auto c = static_cast<std::unique_ptr<mongocxx::cursor> *>(cursor);

	if (!c || !c->get()) {
		logger_->log_error("MongoDB", "mongodb-cursor-more: got invalid cursor");
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

	return CLIPS::Value((*c)->more() ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
}
*/

CLIPS::Value
LLSFRefBox::clips_mongodb_cursor_next(void *cursor)
{
	auto c = static_cast<std::unique_ptr<mongocxx::cursor> *>(cursor);

	if (!c || !c->get()) {
		logger_->log_error("MongoDB", "mongodb-cursor-next: got invalid cursor");
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

	auto it = (*c)->begin();
	if (it == (*c)->end()) {
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}
	document doc{};
	doc.append(bsoncxx::builder::concatenate(*it));
	return CLIPS::Value(new bsoncxx::document::value(doc.extract()));
}

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
		logger_->log_error("MongoDB",
		                   "mongodb-bson-get: cannot get field '%s' from document: %s",
		                   field_name.c_str(),
		                   bsoncxx::to_json(doc->view()).c_str());
		return CLIPS::Value("FALSE", CLIPS::TYPE_SYMBOL);
	}

	switch (element->type()) {
	case bsoncxx::type::k_double: return CLIPS::Value(element->get_double());
	case bsoncxx::type::k_utf8: return CLIPS::Value(element->get_utf8().value.to_string());
	case bsoncxx::type::k_bool:
		return CLIPS::Value(element->get_bool() ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
	case bsoncxx::type::k_null: return CLIPS::Value("nil", CLIPS::TYPE_SYMBOL);

	case bsoncxx::type::k_int32: return CLIPS::Value(element->get_int32());
	case bsoncxx::type::k_int64: return CLIPS::Value(element->get_int64());
	case bsoncxx::type::k_document: {
		return CLIPS::Value(new bsoncxx::document::value(element->get_document().view()));
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
		logger_->log_error("MongoDB",
		                   "mongodb-bson-get-array: cannot get field '%s' from document: %s",
		                   field_name.c_str(),
		                   bsoncxx::to_json(doc->view()).c_str());
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
		case bsoncxx::type::k_double: rv.push_back(CLIPS::Value(element->get_double())); break;
		case bsoncxx::type::k_utf8:
			rv.push_back(CLIPS::Value(element->get_utf8().value.to_string()));
			break;
		case bsoncxx::type::k_bool:
			CLIPS::Value(element->get_bool() ? "TRUE" : "FALSE", CLIPS::TYPE_SYMBOL);
			break;
		case bsoncxx::type::k_int32: rv.push_back(CLIPS::Value(element->get_int32())); break;
		case bsoncxx::type::k_int64: rv.push_back(CLIPS::Value(element->get_int64())); break;
		case bsoncxx::type::k_document: {
			rv.push_back(CLIPS::Value(new bsoncxx::document::value(element->get_document().view())));
			break;
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
		logger_->log_error("MongoDB",
		                   "mongodb-bson-get-time: cannot get field '%s' from document: %s",
		                   field_name.c_str(),
		                   bsoncxx::to_json(doc->view()).c_str());
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
		logger_->log_error("MongoDB",
		                   "mongodb-bson-get-time: field %s is not a time",
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

/** Start the timer for another run. */
void
LLSFRefBox::start_timer()
{
	timer_last_ = boost::posix_time::microsec_clock::local_time();
	timer_.expires_from_now(boost::posix_time::milliseconds(cfg_timer_interval_));
	timer_.async_wait(boost::bind(&LLSFRefBox::handle_timer, this, boost::asio::placeholders::error));
}

/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBox::handle_timer(const boost::system::error_code &error)
{
	if (!error) {
		/*
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    long ms = (now - timer_last_).total_milliseconds();
    timer_last_ = now;
    */

		//sps_read_rfids();

		{
			//std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
			fawkes::MutexLocker lock(&clips_mutex_);

			clips_->assert_fact("(time (now))");
			clips_->refresh_agenda();
			clips_->run();
		}

		timer_.expires_at(timer_.expires_at() + boost::posix_time::milliseconds(cfg_timer_interval_));
		timer_.async_wait(
		  boost::bind(&LLSFRefBox::handle_timer, this, boost::asio::placeholders::error));
	}
}

void
LLSFRefBox::clips_add_machine(const std::string &machine_name)
{
	std::string cfg_prefix = "/llsfrb/mps/stations/" + machine_name + "/";
	auto        old_mps    = mps_.find(machine_name);
	if (old_mps != mps_.end()) {
		old_mps->second.reset();
	}
	bool active = true;
	try {
		active = config_->get_bool((cfg_prefix + "active").c_str());
	} catch (Exception &e) {
	} // ignored, assume enabled

	if (active) {
		std::string  mpstype = config_->get_string((cfg_prefix + "type").c_str());
		std::string  mpsip   = config_->get_string((cfg_prefix + "host").c_str());
		unsigned int port    = config_->get_uint((cfg_prefix + "port").c_str());

		std::string connection_string = "plc";
		try {
			// common setting for all machines
			connection_string = config_->get_string("/llsfrb/mps/connection");
		} catch (Exception &e) {
		}
		try {
			// machine-specific setting
			connection_string = config_->get_string((cfg_prefix + "connection").c_str());
		} catch (Exception &e) {
		}

		std::string log_path = "";
		try {
			log_path = config_->get_string("/llsfrb/log/mps_dir");
		} catch (Exception &e) {
		}

		if (log_path != "") {
			stdfs::create_directory(log_path);
			std::string log_suffix = machine_name + ".log";
			try {
				log_suffix = config_->get_string((cfg_prefix + "/log_file").c_str());
			} catch (Exception &e) {
			}
			log_path += "/" + log_suffix;
		}

		MachineFactory mps_factory(config_);
		auto           mps =
		  mps_factory.create_machine(machine_name, mpstype, mpsip, port, log_path, connection_string);
		mps->register_ready_callback([this, machine_name](bool ready) {
			fawkes::MutexLocker clips_lock(&clips_mutex_);
			clips_->assert_fact_f("(mps-status-feedback %s READY %s)",
			                      machine_name.c_str(),
			                      ready ? "TRUE" : "FALSE");
		});
		mps->register_busy_callback([this, machine_name](bool busy) {
			fawkes::MutexLocker clips_lock(&clips_mutex_);
			clips_->assert_fact_f("(mps-status-feedback %s BUSY %s)",
			                      machine_name.c_str(),
			                      busy ? "TRUE" : "FALSE");
		});
		mps->register_barcode_callback([this, machine_name](unsigned long barcode) {
			fawkes::MutexLocker clips_lock(&clips_mutex_);
			clips_->assert_fact_f("(mps-status-feedback %s BARCODE %u)", machine_name.c_str(), barcode);
		});
		if (mpstype == "RS") {
			RingStation *rs = dynamic_cast<RingStation *>(mps.get());
			if (!rs) {
				throw Exception("Expected MPS %s to be of type RingStation", machine_name.c_str());
			}
			rs->register_slide_callback([this, machine_name](unsigned int counter) {
				fawkes::MutexLocker clips_lock(&clips_mutex_);
				clips_->assert_fact_f("(mps-status-feedback %s SLIDE-COUNTER %u)",
				                      machine_name.c_str(),
				                      counter);
			});
		}
		mps_[machine_name] = std::move(mps);
	}
}

/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBox::handle_signal(const boost::system::error_code &error, int signum)
{
	timer_.cancel();
	io_service_.stop();
	if (!error) {
		if (signum == SIGUSR1) {
			return_code_ = LLSFRefBox::RESTART_CODE;
		} else {
			return_code_ = 0;
		}
	} else {
		return_code_ = 1;
	}
}

/** Run the application.
 * @return return code, 0 if no error, error code otherwise
 */
int
LLSFRefBox::run()
{
#if BOOST_ASIO_VERSION >= 100601
	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM, SIGUSR1);

	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(boost::bind(&LLSFRefBox::handle_signal,
	                               this,
	                               boost::asio::placeholders::error,
	                               boost::asio::placeholders::signal_number));
#else
	g_refbox = this;
	signal(SIGINT, llsfrb::handle_signal);
#endif

	start_timer();
	io_service_.run();
	if (return_code_ != LLSFRefBox::RESTART_CODE) {
		google::protobuf::ShutdownProtobufLibrary();
	}

	return return_code_;
}

#ifdef HAVE_WEBSOCKETS
/** Setup websocket related CLIPS functions. */
void
LLSFRefBox::setup_clips_websocket()
{
	fawkes::MutexLocker lock(&clips_mutex_);

	//tell CLIPS that the websocket rules should be considered
	clips_->build("(deffacts have-feature-websocket (have-feature websocket))");

	//define the functions called by CLIPS

	clips_->add_function("ws-send-attention-message",
	                     sigc::slot<void, std::string, std::string, int, float>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_attention_message)));

	clips_->add_function("ws-create-GameState",
	                     sigc::slot<void>(sigc::mem_fun(*(backend_->get_data()),
	                                                    &websocket::Data::log_push_game_state)));
	clips_->add_function("ws-create-CfgPreset",
	                     sigc::slot<void, std::string, std::string>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_cfg_preset)));
	clips_->add_function("ws-create-TimeInfo",
	                     sigc::slot<void>(sigc::mem_fun(*(backend_->get_data()),
	                                                    &websocket::Data::log_push_time_info)));

	clips_->add_function("ws-create-RobotInfo",
	                     sigc::slot<void, int, std::string>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_robot_info)));
	clips_->add_function("ws-create-AgentTaskInfo",
	                     sigc::slot<void, int, int>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_agent_task_info)));

	clips_->add_function("ws-create-MachineInfo",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_machine_info)));

	clips_->add_function("ws-create-RingInfo",
	                     sigc::slot<void>(sigc::mem_fun(*(backend_->get_data()),
	                                                    &websocket::Data::log_push_ring_spec)));

	clips_->add_function("ws-create-WorkpieceInfo",
	                     sigc::slot<void, int>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_workpiece_info)));

	clips_->add_function("ws-create-OrderInfo",
	                     sigc::slot<void, int>(sigc::mem_fun(*(backend_->get_data()),
	                                                         &websocket::Data::log_push_order_info)));

	clips_->add_function("ws-create-Points",
	                     sigc::slot<void>(sigc::mem_fun(*(backend_->get_data()),
	                                                    &websocket::Data::log_push_points)));
	clips_->add_function("ws-create-Config",
	                     sigc::slot<void, std::string>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_config)));

	clips_->add_function("ws-create-OrderInfo-via-delivery",
	                     sigc::slot<void, int>(
	                       sigc::mem_fun(*(backend_->get_data()),
	                                     &websocket::Data::log_push_order_info_via_delivery)));

	clips_->add_function("ws-create-KnownTeams",
	                     sigc::slot<void>(sigc::mem_fun(*(backend_->get_data()),
	                                                    &websocket::Data::log_push_known_teams)));

	//define functions that set facts in the CLIPS environment to control the refbox
	backend_->get_data()->clips_set_cfg_preset = [this](const std::string &category,
	                                                    const std::string &preset) {
		std::string cfg_file = std::string(CONFDIR) + "/" + category + "/" + preset + ".yaml";
		if (fs::exists(fs::path(cfg_file))) {
			YamlConfiguration tmp_config = YamlConfiguration(CONFDIR);
			tmp_config.load(cfg_file.c_str());
			std::shared_ptr<Configuration::ValueIterator> v(tmp_config.search("/"));
			fawkes::MutexLocker                           clips_lock(&clips_mutex_);
			while (v->next()) {
				clips_assert_confval(v);
			}
		} else {
			logger_->log_error("Websocket",
			                   "Received invalid config preset (%s, %s)",
			                   category.c_str(),
			                   preset.c_str());
		}
	};
	backend_->get_data()->clips_set_gamestate = [this](std::string state_string) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(net-SetGameState %s)", state_string.c_str());
	};
	backend_->get_data()->clips_set_gamephase = [this](std::string phase_string) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(net-SetGamePhase %s)", phase_string.c_str());
	};
	backend_->get_data()->clips_randomize_field = [this]() {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(net-RandomizeField)");
	};
	backend_->get_data()->clips_set_confval = [this](std::string path, std::string value) {
		std::string type = config_->get_type(path);
		if (type == "string") {
			config_->set_string(path.c_str(), value);
		} else if (type == "bool") {
			if (value == "true") {
				config_->set_bool(path.c_str(), true);
			} else if (value == "false") {
				config_->set_bool(path.c_str(), false);
			} else {
				logger_->log_error("Websocket",
				                   "Received unexpected value %s for %s, expected true or false",
				                   value.c_str(),
				                   path.c_str());
				return;
			}
		} else if (type == "float") {
			try {
				float num = std::stof(value);
				config_->set_float(path.c_str(), num);
			} catch (std::exception &e) {
				logger_->log_error("Websocket",
				                   "Received unexpected value %s for %s, expected float %s",
				                   value.c_str(),
				                   path.c_str(),
				                   e.what());
				return;
			}
		} else if (type == "unsigned int") {
			try {
				unsigned int num = std::stoul(value, nullptr, 10);
				config_->set_uint(path.c_str(), num);
			} catch (std::exception &e) {
				logger_->log_error("Websocket",
				                   "Received unexpected value %s for %s, expected uint %s",
				                   value.c_str(),
				                   path.c_str(),
				                   e.what());
				return;
			}
		} else if (type == "int") {
			try {
				int num = std::stoi(value);
				config_->set_int(path.c_str(), num);
			} catch (std::exception &e) {
				logger_->log_error("Websocket",
				                   "Received unexpected value %s for %s, expected int",
				                   value.c_str(),
				                   path.c_str());
				return;
			}
		} else {
			logger_->log_error("Websocket",
			                   "Setting of type %s via frontend is not supported",
			                   type.c_str());
		}
		fawkes::MutexLocker  clips_lock(&clips_mutex_);
		CLIPS::Fact::pointer fact = clips_->get_facts();
		while (fact) {
			if (fact->get_template()->name() == "confval") {
				try {
					if (fact->slot_value("path")[0].as_string() == path) {
						fact->retract();
						break;
					}
				} catch (Exception &e) {
					logger_->log_error("Websocket", "can't access path slot of confval fact");
				}
			}
			fact = fact->next();
		}
		std::shared_ptr<Configuration::ValueIterator> v(config_->search(path.c_str()));
		if (v->valid()) {
			clips_assert_confval(v);
		} else {
			logger_->log_error("Websocket", "Failed to find config ", path.c_str());
		}
	};
	backend_->get_data()->clips_set_teamname = [this](std::string color_string,
	                                                  std::string name_string) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(net-SetTeamName %s \"%s\")", color_string.c_str(), name_string.c_str());
	};
	backend_->get_data()->clips_confirm_delivery =
	  [this](int delivery_id, bool correctness, int order_id, std::string team_color) {
		  fawkes::MutexLocker clips_lock(&clips_mutex_);
		  clips_->assert_fact_f("(order-ConfirmDelivery %d %s %d %s)",
		                        delivery_id,
		                        correctness ? "TRUE" : "FALSE",
		                        order_id,
		                        team_color.c_str());
	  };
	backend_->get_data()->clips_set_order_delivered = [this](std::string team_color, int order_id) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(order-SetOrderDelivered %s %d)", team_color.c_str(), order_id);
	};
	backend_->get_data()->clips_production_machine_add_base = [this](std::string mname) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(production-MachineAddBase %s)", mname.c_str());
	};
	backend_->get_data()->clips_production_set_machine_state = [this](std::string mname,
	                                                                  std::string state) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(production-SetMachineState %s %s)", mname.c_str(), state.c_str());
	};
	backend_->get_data()->clips_robot_set_robot_maintenance =
	  [this](int robot_number, std::string team_color, bool maintenance) {
		  fawkes::MutexLocker clips_lock(&clips_mutex_);
		  clips_->assert_fact_f("(robot-SetRobotMaintenance %d %s %s)",
		                        robot_number,
		                        team_color.c_str(),
		                        maintenance ? "TRUE" : "FALSE");
	  };
	backend_->get_data()->clips_production_reset_machine_by_team = [this](std::string machine_name,
	                                                                      std::string team_color) {
		fawkes::MutexLocker clips_lock(&clips_mutex_);
		clips_->assert_fact_f("(ws-reset-machine-message %s %s)",
		                      machine_name.c_str(),
		                      team_color.c_str());
	};
	backend_->get_data()->clips_add_points_team = [this](int         points,
	                                                     std::string team_color,
	                                                     float       game_time,
	                                                     std::string phase,
	                                                     std::string reason) {
		if ((team_color == "CYAN" || team_color == "MAGENTA")
		    && (phase == "EXPLORATION" || phase == "PRODUCTION")) {
			fawkes::MutexLocker clips_lock(&clips_mutex_);
			clips_->assert_fact_f(
			  "(points (points %d) (team %s) (game-time %f) (phase %s) (reason \"%s\"))",
			  points,
			  team_color.c_str(),
			  game_time,
			  phase.c_str(),
			  reason.c_str());
		} else {
			logger_->log_error("Websocket",
			                   ": Received invalid points, expected team-color CYAN|MAGENTA and phase "
			                   "EXPLORATION|PRODUCTION, but got %s and %s",
			                   team_color.c_str(),
			                   phase.c_str());
		}
	};
}

#endif

} // end of namespace llsfrb
