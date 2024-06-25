
// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  mongodb_log_logger.cpp - MongoDB text logger
 *
 *  Created: Fri Jun 07 16:11:51 2013
 *           (based on Fawkes'  mongodb_log_logger_thread.cpp)
 *  Copyright  2006-2013  Tim Niemueller [www.niemueller.de]
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

#include <core/threading/mutex.h>
#include <core/threading/mutex_locker.h>
#include <mongodb_log/mongodb_log_logger.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <chrono>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/uri.hpp>
#include <string>

using namespace mongo;
using namespace fawkes;

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::sub_document;

/** @class MongoDBLogLogger <mongodb_log/mongodb_log_logger.h>
 * Thread that provides a logger writing to MongoDB.
 * This thread provides a logger, which writes log information to a
 * MongoDB collection.
 * @author Tim Niemueller
 */

/** Constructor. */
MongoDBLogLogger::MongoDBLogLogger(std::string host_port, std::string collection)
{
	mutex_ = new Mutex();
	std::string uri{"mongodb://" + host_port};
	client_     = mongocxx::client{mongocxx::uri{"mongodb://" + host_port}};
	collection_ = client_["rcll"][collection];
}

/** Destructor. */
MongoDBLogLogger::~MongoDBLogLogger()
{
	delete mutex_;
}

void
MongoDBLogLogger::insert_message(LogLevel ll, const char *component, const char *format, va_list va)
{
	if (log_level <= ll) {
		MutexLocker    lock(mutex_);
		struct timeval now;
		gettimeofday(&now, NULL);
		//Date_t nowd = now.tv_sec * 1000 + now.tv_usec / 1000;

		char *msg;
		if (vasprintf(&msg, format, va) == -1) {
			// Cannot do anything useful, drop log message
			return;
		}

		document doc{};
		doc.append(kvp("time", bsoncxx::types::b_date(std::chrono::system_clock::now())));
		switch (ll) {
		case LL_DEBUG: doc.append(kvp("level", "DEBUG")); break;
		case LL_INFO: doc.append(kvp("level", "INFO")); break;
		case LL_WARN: doc.append(kvp("level", "WARN")); break;
		case LL_ERROR: doc.append(kvp("level", "ERROR")); break;
		default: doc.append(kvp("level", "UNKN")); break;
		}
		doc.append(kvp("component", component));
		doc.append(kvp("message", msg));
		collection_.insert_one(doc.view());
		free(msg);
	}
}

void
MongoDBLogLogger::insert_message(LogLevel ll, const char *component, Exception &e)
{
	if (log_level <= ll) {
		MutexLocker lock(mutex_);

		for (Exception::iterator i = e.begin(); i != e.end(); ++i) {
			document doc{};
			switch (ll) {
			case LL_DEBUG: doc.append(kvp("level", "DEBUG")); break;
			case LL_INFO: doc.append(kvp("level", "INFO")); break;
			case LL_WARN: doc.append(kvp("level", "WARN")); break;
			case LL_ERROR: doc.append(kvp("level", "ERROR")); break;
			default: doc.append(kvp("level", "UNKN")); break;
			}
			doc.append(kvp("time", bsoncxx::types::b_date(std::chrono::system_clock::now())));
			doc.append(kvp("component", component));
			doc.append(kvp("message", std::string("[EXCEPTION] ") + *i));
			try {
				collection_.insert_one(doc.view());
			} catch (mongocxx::operation_exception &) {
			} // ignored
		}
	}
}

void
MongoDBLogLogger::vlog_debug(const char *component, const char *format, va_list va)
{
	insert_message(LL_DEBUG, component, format, va);
}

void
MongoDBLogLogger::vlog_info(const char *component, const char *format, va_list va)
{
	insert_message(LL_INFO, component, format, va);
}

void
MongoDBLogLogger::vlog_warn(const char *component, const char *format, va_list va)
{
	insert_message(LL_WARN, component, format, va);
}

void
MongoDBLogLogger::vlog_error(const char *component, const char *format, va_list va)
{
	insert_message(LL_ERROR, component, format, va);
}

void
MongoDBLogLogger::log_debug(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	insert_message(LL_DEBUG, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::log_info(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	insert_message(LL_INFO, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::log_warn(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	insert_message(LL_WARN, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::log_error(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	insert_message(LL_ERROR, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::log_debug(const char *component, Exception &e)
{
	insert_message(LL_DEBUG, component, e);
}

void
MongoDBLogLogger::log_info(const char *component, Exception &e)
{
	insert_message(LL_INFO, component, e);
}

void
MongoDBLogLogger::log_warn(const char *component, Exception &e)
{
	insert_message(LL_WARN, component, e);
}

void
MongoDBLogLogger::log_error(const char *component, Exception &e)
{
	insert_message(LL_ERROR, component, e);
}

void
MongoDBLogLogger::tlog_insert_message(LogLevel        ll,
                                      struct timeval *t,
                                      const char     *component,
                                      const char     *format,
                                      va_list         va)
{
	if (log_level <= ll) {
		MutexLocker lock(mutex_);
		char       *msg;
		if (vasprintf(&msg, format, va) == -1) {
			return;
		}
		document doc{};
		switch (ll) {
		case LL_DEBUG: doc.append(kvp("level", "DEBUG")); break;
		case LL_INFO: doc.append(kvp("level", "INFO")); break;
		case LL_WARN: doc.append(kvp("level", "WARN")); break;
		case LL_ERROR: doc.append(kvp("level", "ERROR")); break;
		default: doc.append(kvp("level", "UNKN")); break;
		}
		doc.append(kvp("component", component));
		doc.append(kvp("time", bsoncxx::types::b_date(std::chrono::system_clock::now())));
		doc.append(kvp("message", msg));
		try {
			collection_.insert_one(doc.view());
		} catch (mongocxx::operation_exception &) {
		} // ignored

		free(msg);

		mutex_->unlock();
	}
}

void
MongoDBLogLogger::tlog_insert_message(LogLevel        ll,
                                      struct timeval *t,
                                      const char     *component,
                                      Exception      &e)
{
	if (log_level <= ll) {
		MutexLocker lock(mutex_);
		for (Exception::iterator i = e.begin(); i != e.end(); ++i) {
			document doc{};
			switch (ll) {
			case LL_DEBUG: doc.append(kvp("level", "DEBUG")); break;
			case LL_INFO: doc.append(kvp("level", "INFO")); break;
			case LL_WARN: doc.append(kvp("level", "WARN")); break;
			case LL_ERROR: doc.append(kvp("level", "ERROR")); break;
			default: doc.append(kvp("level", "UNKN")); break;
			}
			doc.append(kvp("component", component));
			doc.append(kvp("time", bsoncxx::types::b_date(std::chrono::system_clock::now())));
			doc.append(kvp("message", std::string("[EXCEPTION] ") + *i));
			try {
				collection_.insert_one(doc.view());
			} catch (mongocxx::operation_exception &) {
			} // ignored
		}
	}
}

void
MongoDBLogLogger::tlog_debug(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	tlog_insert_message(LL_DEBUG, t, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::tlog_info(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	tlog_insert_message(LL_INFO, t, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::tlog_warn(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	tlog_insert_message(LL_WARN, t, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::tlog_error(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	tlog_insert_message(LL_ERROR, t, component, format, arg);
	va_end(arg);
}

void
MongoDBLogLogger::tlog_debug(struct timeval *t, const char *component, Exception &e)
{
	tlog_insert_message(LL_DEBUG, t, component, e);
}

void
MongoDBLogLogger::tlog_info(struct timeval *t, const char *component, Exception &e)
{
	tlog_insert_message(LL_INFO, t, component, e);
}

void
MongoDBLogLogger::tlog_warn(struct timeval *t, const char *component, Exception &e)
{
	tlog_insert_message(LL_WARN, t, component, e);
}

void
MongoDBLogLogger::tlog_error(struct timeval *t, const char *component, Exception &e)
{
	tlog_insert_message(LL_ERROR, t, component, e);
}

void
MongoDBLogLogger::vtlog_debug(struct timeval *t,
                              const char     *component,
                              const char     *format,
                              va_list         va)
{
	tlog_insert_message(LL_DEBUG, t, component, format, va);
}

void
MongoDBLogLogger::vtlog_info(struct timeval *t,
                             const char     *component,
                             const char     *format,
                             va_list         va)
{
	tlog_insert_message(LL_INFO, t, component, format, va);
}

void
MongoDBLogLogger::vtlog_warn(struct timeval *t,
                             const char     *component,
                             const char     *format,
                             va_list         va)
{
	tlog_insert_message(LL_WARN, t, component, format, va);
}

void
MongoDBLogLogger::vtlog_error(struct timeval *t,
                              const char     *component,
                              const char     *format,
                              va_list         va)
{
	tlog_insert_message(LL_ERROR, t, component, format, va);
}
