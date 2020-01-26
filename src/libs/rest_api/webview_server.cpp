
/***************************************************************************
 *  webview_server.cpp - Thread that handles web interface requests
 *
 *  Created: Mon Oct 13 17:51:31 2008 (I5 Developer's Day)
 *  Copyright  2006-2018  Tim Niemueller [www.niemueller.de]
 *             2020       Mostafa Gomaa  [gomaa@kbsg.rwth-aachen.de]
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

#include "webview_server.h"
#include "rest_processor.h"
#include "service_browse_handler.h"
#include "user_verifier.h"
#include "clips_rest_api/clips-rest-api.h"

#include <core/exceptions/system.h>
#include <core/version.h>
#include <sys/wait.h>
#include <utils/misc/string_conversions.h>
#include <utils/system/file.h>
#include <utils/system/hostinfo.h>
#include <config/config.h>
#include <logging/logger.h>

#include <webview/page_reply.h>
#include <webview/request_dispatcher.h>
#include <webview/server.h>
#include <webview/url_manager.h>
#include <webview/nav_manager.h>
#include <webview/request_manager.h>
#include <webview/rest_api_manager.h>

#include <netcomm/service_discovery/service_browser.h>
#include <netcomm/service_discovery/service_publisher.h>

#include <clipsmm.h>

using namespace fawkes;

/** @class WebviewServer "webview_server.h"
 * Webview Server.
 * This thread runs the HTTP server and handles requests via the
 * WebRequestDispatcher.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param enable_tp true to enable thread pool setting the thread to
 * wait-for-wakeup mode, falso to run request processing in this
 * thread.
 */
namespace llsfrb{

WebviewServer::WebviewServer(bool enable_tp,
                             fawkes::NetworkNameResolver *nnresolver,
                             fawkes::ServicePublisher    *service_publisher,
                             fawkes::ServiceBrowser      *service_browser,
					    fawkes::Mutex               &clips_mutex,
					    CLIPS::Environment          *env,
                             Configuration               *config,
                             Logger * logger)
: fawkes::Thread("WebviewServer", Thread::OPMODE_CONTINUOUS)
{
	cfg_use_thread_pool_ = enable_tp;

	if (!enable_tp)
		set_prepfin_conc_loop(true);

	nnresolver_ = nnresolver;
	service_publisher_ = service_publisher;
	service_browser_   = service_browser;
	config_ = config;
	logger_ =logger;

	url_manager_       = new WebUrlManager();
	request_manager_   = new WebRequestManager();
	rest_api_manager_  = new WebviewRestApiManager();
	clips_rest_api_    = new ClipsRestApi(env, clips_mutex, rest_api_manager_, logger_);
}

WebviewServer::~WebviewServer()
{
}

void
WebviewServer::init()
{
	logger_->log_info("WebviewServer", "Initializing thread");


	cfg_port_ = config_->get_uint("/webview/port");

	WebReply::set_caching_default(config_->get_bool("/webview/client_side_caching"));

	webview_service_        = NULL;
	service_browse_handler_ = NULL;
	dispatcher_             = NULL;

	cfg_use_ipv4_ = true;
	cfg_use_ipv6_ = false;


	/*
	cfg_use_tls_ = false;
	try {
		cfg_use_tls_ = config_->get_bool("/webview/tls/enable");
	} catch (Exception &e) {
	}

	cfg_use_ipv4_ = config_->get_bool("/network/ipv4/enable");
	cfg_use_ipv6_ = config_->get_bool("/network/ipv6/enable");

	if (cfg_use_tls_) {
		cfg_tls_create_ = false;
		try {
			cfg_tls_create_ = config_->get_bool("/webview/tls/create");
		} catch (Exception &e) {
		}

		cfg_tls_key_  = config_->get_string("/webview/tls/key-file");
		cfg_tls_cert_ = config_->get_string("/webview/tls/cert-file");

		try {
			cfg_tls_cipher_suite_ = config_->get_string("/webview/tls/cipher-suite");
			logger_->log_debug(name(), "Using cipher suite %s", cfg_tls_cipher_suite_.c_str());
		} catch (Exception &e) {
		}

		if (cfg_tls_key_[0] != '/')
			cfg_tls_key_ = std::string(CONFDIR "/") + cfg_tls_key_;

		if (cfg_tls_cert_[0] != '/')
			cfg_tls_cert_ = std::string(CONFDIR "/") + cfg_tls_cert_;

		logger_->log_debug(name(),
		                  "Key file: %s  Cert file: %s",
		                  cfg_tls_key_.c_str(),
		                  cfg_tls_cert_.c_str());

		if (!File::exists(cfg_tls_key_.c_str())) {
			if (File::exists(cfg_tls_cert_.c_str())) {
				throw Exception("Key file %s does not exist, but certificate file %s "
				                "does",
				                cfg_tls_key_.c_str(),
				                cfg_tls_cert_.c_str());
			} else if (cfg_tls_create_) {
				tls_create(cfg_tls_key_.c_str(), cfg_tls_cert_.c_str());
			} else {
				throw Exception("Key file %s does not exist", cfg_tls_key_.c_str());
			}
		} else if (!File::exists(cfg_tls_cert_.c_str())) {
			throw Exception("Certificate file %s does not exist, but key file %s "
			                "does",
			                cfg_tls_key_.c_str(),
			                cfg_tls_cert_.c_str());
		}
	}

	if (cfg_use_thread_pool_) {
		cfg_num_threads_ = config_->get_uint("/webview/thread-pool/num-threads");
	}

	cfg_use_basic_auth_ = false;
	try {
		cfg_use_basic_auth_ = config_->get_bool("/webview/use_basic_auth");
	} catch (Exception &e) {
	}
	cfg_basic_auth_realm_ = "Refbox webview";
	try {
		cfg_basic_auth_realm_ = config_->get_bool("/webview/basic_auth_realm");
	} catch (Exception &e) {
	}

	cfg_access_log_ = "";
	try {
		cfg_access_log_ = config_->get_string("/webview/access_log");
	} catch (Exception &e) {
	}
	*/

	bool cfg_cors_allow_all = false;
	try {
		cfg_cors_allow_all = config_->get_bool("/webview/cors/allow/all");
	} catch (Exception &e) {
	}
	std::vector<std::string> cfg_cors_origins;
	try {
		cfg_cors_origins = config_->get_strings("/webview/cors/allow/origins");
	} catch (Exception &e) {
	}
	unsigned int cfg_cors_max_age = 0;
	try {
		cfg_cors_max_age = config_->get_uint("/webview/cors/max-age");
	} catch (Exception &e) {
	}

	webview_service_ =
	  new NetworkService(nnresolver_, "Refbox Webview on %h", "_http._tcp", cfg_port_);
	webview_service_->add_txt("refboxver=%u.%u.%u",
	                          0,
	                          0,
	                          0);
	service_browse_handler_ = new WebviewServiceBrowseHandler(logger_, webview_service_);

	dispatcher_ = new WebRequestDispatcher(url_manager_);

	try {
		webserver_ = new WebServer(cfg_port_, dispatcher_, logger_);

		(*webserver_)
		  .setup_ipv(cfg_use_ipv4_, cfg_use_ipv6_)
		  .setup_cors(cfg_cors_allow_all, std::move(cfg_cors_origins), cfg_cors_max_age);

		/*
		if (cfg_use_tls_) {
			webserver_->setup_tls(cfg_tls_key_.c_str(),
			                      cfg_tls_cert_.c_str(),
			                      cfg_tls_cipher_suite_.empty() ? NULL : cfg_tls_cipher_suite_.c_str());
		}

		if (cfg_use_thread_pool_) {
			webserver_->setup_thread_pool(cfg_num_threads_);
		}

		if (cfg_use_basic_auth_) {
			user_verifier_ = new WebviewUserVerifier(config_, logger_);
			webserver_->setup_basic_auth(cfg_basic_auth_realm_.c_str(), user_verifier_);
		}
          */

		webserver_->setup_request_manager(request_manager_);

	    /*
		if (cfg_access_log_ != "") {
			logger_->log_debug(name(), "Setting up access log %s", cfg_access_log_.c_str());
			webserver_->setup_access_log(cfg_access_log_.c_str());
		}
		*/

	} catch (Exception &e) {
		delete webview_service_;
		delete service_browse_handler_;
		delete dispatcher_;
		throw;
	}
	rest_processor_ =
	  new WebviewRESTRequestProcessor(url_manager_, rest_api_manager_, logger_);

	try {
		cfg_explicit_404_ = config_->get_strings("/webview/explicit-404");
		for (const auto &u : cfg_explicit_404_) {
			url_manager_->add_handler(WebRequest::METHOD_GET,
			                                 u,
			                                 std::bind(&WebviewServer::produce_404, this),
			                                 10000);
		}
	} catch (Exception &e) {
	} // ignored, no explicit 404

	std::string afs;
	if (cfg_use_ipv4_ && cfg_use_ipv6_) {
		afs = "IPv4,IPv6";
	} else if (cfg_use_ipv4_) {
		afs = "IPv4";
	} else if (cfg_use_ipv6_) {
		afs = "IPv6";
	}
	webserver_->start();
	logger_->log_info("WebviewServer",
	                 "Listening for HTTP connections on port %u (%s)",
	                 cfg_port_,
	                 afs.c_str());

	service_publisher_->publish_service(webview_service_);
	service_browser_->watch_service("_http._tcp", service_browse_handler_);

	try{
	  //Clips Rest api
	  clips_rest_api_->init();
	  clips_rest_api_->start();
	} catch (Exception &e) {
	    throw;
	}

}

void
WebviewServer::finalize()
{

    clips_rest_api_->cancel();
    clips_rest_api_->join();
    delete clips_rest_api_;

    try {
		service_publisher_->unpublish_service(webview_service_);
	} catch (Exception &e) {
	} // ignored, can happen if avahi-daemon not running
	try {
		service_browser_->unwatch_service("_http._tcp", service_browse_handler_);
	} catch (Exception &e) {
	} // ignored, can happen if avahi-daemon not running

	for (const auto &u : cfg_explicit_404_) {
		url_manager_->remove_handler(WebRequest::METHOD_GET, u);
	}

	delete webserver_;

	delete webview_service_;
	delete service_browse_handler_;

	delete dispatcher_;
	delete rest_processor_;
	dispatcher_ = NULL;

	delete url_manager_;
	delete request_manager_;
	delete rest_api_manager_;
}

void
WebviewServer::loop()
{
	if (!cfg_use_thread_pool_)
		webserver_->process();
}

void
WebviewServer::tls_create(const char *tls_key_file, const char *tls_cert_file)
{
	logger_->log_info(name(),
	                 "Creating TLS key and certificate. "
	                 "This may take a while...");
	HostInfo h;

	char *cmd;
	if (asprintf(&cmd,
	             "openssl req -new -x509 -batch -nodes -days 365 "
	             "-subj \"/C=XX/L=World/O=Fawkes/CN=%s.local\" "
	             "-out \"%s\" -keyout \"%s\" >/dev/null 2>&1",
	             h.short_name(),
	             tls_cert_file,
	             tls_key_file)
	    == -1) {
		throw OutOfMemoryException("Webview/TLS: Could not generate OpenSSL string");
	}

	int status = system(cmd);
	free(cmd);

	if (WEXITSTATUS(status) != 0) {
		throw Exception("Failed to auto-generate key/certificate pair");
	}
}

WebReply *
WebviewServer::produce_404()
{
	return new StaticWebReply(WebReply::HTTP_NOT_FOUND, "Not found\n");
}

}
