
/***************************************************************************
 *  websocket.h - Refbox websocket logger
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

#ifndef __UTILS_LOGGING_WEBSOCKET_H_
#define __UTILS_LOGGING_WEBSOCKET_H_

#include <logging/logger.h>
#include <websocket/data.h>

#include <boost/format.hpp>
#include <ctime>

namespace llsfrb {

class Mutex;

class WebsocketLogger : public Logger
{
public:
	WebsocketLogger(std::shared_ptr<websocket::Data> data, LogLevel log_level = LL_DEBUG);
	virtual ~WebsocketLogger();

	virtual void log_debug(const char *component, const char *format, ...);
	virtual void log_info(const char *component, const char *format, ...);
	virtual void log_warn(const char *component, const char *format, ...);
	virtual void log_error(const char *component, const char *format, ...);

	virtual void vlog_debug(const char *component, const char *format, va_list va);
	virtual void vlog_info(const char *component, const char *format, va_list va);
	virtual void vlog_warn(const char *component, const char *format, va_list va);
	virtual void vlog_error(const char *component, const char *format, va_list va);

	virtual void log_debug(const char *component, fawkes::Exception &e);
	virtual void log_info(const char *component, fawkes::Exception &e);
	virtual void log_warn(const char *component, fawkes::Exception &e);
	virtual void log_error(const char *component, fawkes::Exception &e);

	virtual void tlog_debug(struct timeval *t, const char *component, const char *format, ...);
	virtual void tlog_info(struct timeval *t, const char *component, const char *format, ...);
	virtual void tlog_warn(struct timeval *t, const char *component, const char *format, ...);
	virtual void tlog_error(struct timeval *t, const char *component, const char *format, ...);

	virtual void tlog_debug(struct timeval *t, const char *component, fawkes::Exception &e);
	virtual void tlog_info(struct timeval *t, const char *component, fawkes::Exception &e);
	virtual void tlog_warn(struct timeval *t, const char *component, fawkes::Exception &e);
	virtual void tlog_error(struct timeval *t, const char *component, fawkes::Exception &e);

	virtual void
	vtlog_debug(struct timeval *t, const char *component, const char *format, va_list va);
	virtual void vtlog_info(struct timeval *t, const char *component, const char *format, va_list va);
	virtual void vtlog_warn(struct timeval *t, const char *component, const char *format, va_list va);
	virtual void
	vtlog_error(struct timeval *t, const char *component, const char *format, va_list va);

private:
	struct ::tm                     *now_s;
	fawkes::Mutex                   *mutex;
	std::shared_ptr<websocket::Data> data_;
	boost::format                    fmt_time;
	std::string                      formatter(const char *format, va_list va);
	std::string                      formatter(const char *format, const char *text);
	void                             build_document(rapidjson::Document *d,
	                                                const char          *component,
	                                                std::string          level,
	                                                std::string          time,
	                                                std::string          message,
	                                                bool                 exception = false);
	void                             build_document(rapidjson::Document *d,
	                                                const char          *component,
	                                                std::string          level,
	                                                std::string          time,
	                                                rapidjson::Value    &messages,
	                                                bool                 exception = false);
};

} // end namespace llsfrb

#endif
