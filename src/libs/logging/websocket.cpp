
/***************************************************************************
 *  websocket.cpp - Refbox websocket logger
 *
 *  Created: Thu Apr 02 12:46:00 2020
 *  Copyright  2020  Daniel Swoboda [swoboda@kbsg.rwth-aachen.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <core/threading/mutex.h>
#include <logging/websocket.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sys/time.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace fawkes;

namespace llsfrb {

/** @class WebsocketLogger <logging/websocket.h>
 * Interface for logging to connected (tcp-)/(web-)socket clients using the websocket backend.
 * The WebsocketLogger will put messages into the message queue of the websocket backend. The
 * messages will then be sent to the connected clients by the backend in order. The sent 
 * messages will be formatted as JSON messages with corresponding metadata.
 *
 * Graphical displaying is managed by the connected client, given the metadata sent with 
 * the log messages that are similar to the information provided by the ConsoleLogger.
 * Most interaction and metadata is based on the ConsoleLogger. 
 *
 * @author Daniel Swoboda
 */

/** Constructor.
 * @param log_level minimum level to log
 */
WebsocketLogger::WebsocketLogger(std::shared_ptr<websocket::Data> data, LogLevel log_level)
: Logger(log_level), data_(data)
{
	now_s    = (struct ::tm *)malloc(sizeof(struct ::tm));
	mutex    = new fawkes::Mutex();
	fmt_time = boost::format("%02d:%02d:%02d.%06ld");
}

/** Destructor. */
WebsocketLogger::~WebsocketLogger()
{
	free(now_s);
	delete mutex;
}

/**
 * @brief Creates std::string out of format cstr and va_list
 * 
 * @param format format string
 * @param va argument list
 * @return std::string resulting string
 */
std::string
WebsocketLogger::formatter(const char *format, va_list va)
{
	char *      tmp;
	std::string str_message;
	if (vasprintf(&tmp, format, va) != -1) {
		str_message = std::string(tmp);
		free(tmp);
	}
	return str_message;
}

/**
 * @brief Creates std::string out of format cstr and argument cstr
 * 
 * @param format format string
 * @param text argument
 * @return std::string resulting string
 */
std::string
WebsocketLogger::formatter(const char *format, const char *text)
{
	char *      tmp;
	std::string str_message;
	if (asprintf(&tmp, format, text) != -1) {
		str_message = std::string(tmp);
		free(tmp);
	}
	return str_message;
}

/**
 * @brief creates a log rapidjson document for client consumption
 * 
 * @param d rapidjson document to be filled
 * @param component cstr, affected component 
 * @param level string, level of the log 
 * @param time string, time of the log
 * @param message string, log message
 * @param exception bool, optional, true if message is exception
 */
void
WebsocketLogger::build_document(rapidjson::Document *d,
                                const char *         component,
                                std::string          level,
                                std::string          time,
                                std::string          message,
                                bool                 exception)
{
	rapidjson::Document::AllocatorType &alloc = d->GetAllocator();

	rapidjson::Value str_time;
	str_time.SetString((time).c_str(), alloc);
	d->AddMember("time", str_time, alloc);

	rapidjson::Value str_component;
	str_component.SetString(rapidjson::StringRef(component));
	d->AddMember("component", str_component, alloc);

	rapidjson::Value str_level;
	str_level.SetString((level).c_str(), alloc);
	d->AddMember("level", str_level, alloc);

	rapidjson::Value str_message;
	str_message.SetString((message).c_str(), alloc);
	d->AddMember("message", str_message, alloc);

	if (exception) {
		d->AddMember("exception", "true", alloc);
	}
}

/**
 * @brief creates a log rapidjson document for client consumption with multiple content messages
 * 
 * @param d rapidjson document to be filled
 * @param component cstr, affected component 
 * @param level string, level of the log 
 * @param time string, time of the log
 * @param messages rapidjson value object, array of messages
 * @param exception bool, optional, true if message is exception
 */
void
WebsocketLogger::build_document(rapidjson::Document *d,
                                const char *         component,
                                std::string          level,
                                std::string          time,
                                rapidjson::Value &   messages,
                                bool                 exception)
{
	rapidjson::Document::AllocatorType &alloc = d->GetAllocator();

	rapidjson::Value str_time;
	str_time.SetString((time).c_str(), alloc);
	d->AddMember("time", str_time, alloc);

	rapidjson::Value str_component;
	str_component.SetString(rapidjson::StringRef(component));
	d->AddMember("component", str_component, alloc);

	rapidjson::Value str_level;
	str_level.SetString((level).c_str(), alloc);
	d->AddMember("level", str_level, alloc);

	d->AddMember("messages", messages, alloc);
}

void
WebsocketLogger::vlog_debug(const char *component, const char *format, va_list va)
{
	if (log_level <= LL_DEBUG) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		build_document(&d,
		               component,
		               "debug",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vlog_info(const char *component, const char *format, va_list va)
{
	if (log_level <= LL_INFO) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		build_document(&d,
		               component,
		               "info",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vlog_warn(const char *component, const char *format, va_list va)
{
	if (log_level <= LL_WARN) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		build_document(&d,
		               component,
		               "warn",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vlog_error(const char *component, const char *format, va_list va)
{
	if (log_level <= LL_ERROR) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		build_document(&d,
		               component,
		               "error",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::log_debug(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vlog_debug(component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::log_info(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vlog_info(component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::log_warn(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vlog_warn(component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::log_error(const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vlog_error(component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::log_debug(const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_DEBUG) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "debug",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::log_info(const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_DEBUG) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "info",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::log_warn(const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_DEBUG) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "warn",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::log_error(const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_DEBUG) {
		struct timeval now;
		gettimeofday(&now, NULL);
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&now.tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "debug",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)now.tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::tlog_debug(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vtlog_debug(t, component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::tlog_info(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vtlog_info(t, component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::tlog_warn(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vtlog_warn(t, component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::tlog_error(struct timeval *t, const char *component, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vtlog_error(t, component, format, arg);
	va_end(arg);
}

void
WebsocketLogger::tlog_debug(struct timeval *t, const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_DEBUG) {
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "debug",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::tlog_info(struct timeval *t, const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_INFO) {
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "info",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::tlog_warn(struct timeval *t, const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_WARN) {
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "warn",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::tlog_error(struct timeval *t, const char *component, fawkes::Exception &e)
{
	if (log_level <= LL_ERROR) {
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Document::AllocatorType &alloc = d.GetAllocator();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		rapidjson::Value messages(rapidjson::kArrayType);
		for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
			rapidjson::Value str_message;
			str_message.SetString((formatter("%s", *i)).c_str(), alloc);
			messages.PushBack(str_message, alloc);
		}

		build_document(&d,
		               component,
		               "error",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               messages,
		               true);
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vtlog_debug(struct timeval *t,
                             const char *    component,
                             const char *    format,
                             va_list         va)
{
	if (log_level <= LL_DEBUG) {
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		build_document(&d,
		               component,
		               "debug",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vtlog_info(struct timeval *t,
                            const char *    component,
                            const char *    format,
                            va_list         va)
{
	if (log_level <= LL_INFO) {
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		build_document(&d,
		               component,
		               "info",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vtlog_warn(struct timeval *t,
                            const char *    component,
                            const char *    format,
                            va_list         va)
{
	if (log_level <= LL_WARN) {
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		build_document(&d,
		               component,
		               "warn",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

void
WebsocketLogger::vtlog_error(struct timeval *t,
                             const char *    component,
                             const char *    format,
                             va_list         va)
{
	if (log_level <= LL_ERROR) {
		rapidjson::Document d;
		d.SetObject();

		mutex->lock();
		localtime_r(&t->tv_sec, now_s);

		build_document(&d,
		               component,
		               "error",
		               boost::str(fmt_time % now_s->tm_hour % now_s->tm_min % now_s->tm_sec
		                          % (long)t->tv_usec),
		               formatter(format, va));
		data_->log_push(d);

		mutex->unlock();
	}
}

} // end namespace llsfrb
