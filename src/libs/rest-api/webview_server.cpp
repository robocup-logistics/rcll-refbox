
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

#include <config/config.h>
#include <core/exceptions/system.h>
#include <core/version.h>
#include <logging/logger.h>
#include <netcomm/service_discovery/service_browser.h>
#include <netcomm/service_discovery/service_publisher.h>
#include <netcomm/utils/resolver.h>
#include <sys/wait.h>
#include <utils/misc/string_conversions.h>
#include <utils/system/file.h>
#include <utils/system/hostinfo.h>
#include <webview/nav_manager.h>
#include <webview/page_reply.h>
#include <webview/request_dispatcher.h>
#include <webview/request_manager.h>
#include <webview/rest_api_manager.h>
#include <webview/server.h>
#include <webview/url_manager.h>

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
namespace llsfrb {

WebviewServer::WebviewServer(bool                                           enable_tp,
                             std::shared_ptr<fawkes::WebviewRestApiManager> rest_api_manager,
                             std::unique_ptr<fawkes::NetworkNameResolver>   nnresolver,
                             std::shared_ptr<fawkes::ServicePublisher>      service_publisher,
                             std::shared_ptr<fawkes::ServiceBrowser>        service_browser,
                             Configuration                                 *config,
                             Logger                                        *logger)
: fawkes::Thread("WebviewServer", Thread::OPMODE_CONTINUOUS),
  rest_api_manager_(rest_api_manager),
  nnresolver_(std::move(nnresolver)),
  service_publisher_(service_publisher),
  service_browser_(service_browser),
  config_(config),
  logger_(logger)
{
	cfg_use_thread_pool_ = enable_tp;

	if (!enable_tp)
		set_prepfin_conc_loop(true);

	webview_url_manager_     = std::make_unique<WebUrlManager>();
	webview_request_manager_ = std::make_unique<WebRequestManager>();

	logger_->log_info("WebviewServer", "Initializing thread");

	cfg_port_ = config_->get_uint("/webview/port");

	WebReply::set_caching_default(config_->get_bool("/webview/client_side_caching"));

	webview_service_        = NULL;
	service_browse_handler_ = NULL;
	dispatcher_             = NULL;

	cfg_use_ipv4_ = config_->get_bool("/webview/network/ipv4/enable");
	cfg_use_ipv6_ = config_->get_bool("/webview/network/ipv6/enable");

	if (cfg_use_thread_pool_) {
		cfg_num_threads_ = config_->get_uint("/webview/thread-pool/num-threads");
	}

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

	webview_service_ = std::make_shared<NetworkService>(nnresolver_.get(),
	                                                    "Refbox Webview on %h",
	                                                    "_http._tcp",
	                                                    cfg_port_);
	webview_service_->add_txt("refboxver=%u.%u.%u", 0, 0, 0);
	service_browse_handler_ =
	  std::make_unique<WebviewServiceBrowseHandler>(logger_, webview_service_);

	dispatcher_ = std::make_unique<WebRequestDispatcher>(webview_url_manager_.get());

	try {
		webserver_ = std::make_unique<WebServer>(cfg_port_, dispatcher_.get(), logger_);

		(*webserver_)
		  .setup_ipv(cfg_use_ipv4_, cfg_use_ipv6_)
		  .setup_cors(cfg_cors_allow_all, std::move(cfg_cors_origins), cfg_cors_max_age);

		webserver_->setup_request_manager(webview_request_manager_.get());

		if (cfg_use_thread_pool_) {
			webserver_->setup_thread_pool(cfg_num_threads_);
		}

	} catch (Exception &e) {
		throw;
	}
	rest_processor_ = std::make_unique<WebviewRESTRequestProcessor>(webview_url_manager_.get(),
	                                                                rest_api_manager_.get(),
	                                                                logger_);

	try {
		cfg_explicit_404_ = config_->get_strings("/webview/explicit-404");
		for (const auto &u : cfg_explicit_404_) {
			webview_url_manager_->add_handler(WebRequest::METHOD_GET,
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

	service_publisher_->publish_service(webview_service_.get());
	service_browser_->watch_service("_http._tcp", service_browse_handler_.get());
}

WebviewServer::~WebviewServer()
{
	try {
		service_publisher_->unpublish_service(webview_service_.get());
	} catch (Exception &e) {
	} // ignored, can happen if avahi-daemon not running
	try {
		service_browser_->unwatch_service("_http._tcp", service_browse_handler_.get());
	} catch (Exception &e) {
	} // ignored, can happen if avahi-daemon not running

	for (const auto &u : cfg_explicit_404_) {
		webview_url_manager_->remove_handler(WebRequest::METHOD_GET, u);
	}

	dispatcher_ = NULL;
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

} // namespace llsfrb
