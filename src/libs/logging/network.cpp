
/***************************************************************************
 *  network.cpp - Fawkes network logger
 *
 *  Created: Sat Dec 15 00:48:52 2007 (after I5 xmas party)
 *  Copyright  2006-2007  Tim Niemueller [www.niemueller.de]
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

#include <logging/network.h>

#include <logging/llsf_log_msgs/LogMessage.pb.h>
#include <protobuf_comm/server.h>

#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace llsfrb {


/** @class NetworkLogger <logging/network.h>
 * Interface for logging to network clients.
 * The NetworkLogger will pipe all output to clients.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param server protobuf server to send to
 * @param log_level minimum level to log
 */
NetworkLogger::NetworkLogger(protobuf_comm::ProtobufStreamServer *server, LogLevel log_level)
  : Logger(log_level), pb_server_(server)
{
  pb_server_->message_register().add_message_type<llsf_log_msgs::LogMessage>();
}


/** Destructor. */
NetworkLogger::~NetworkLogger()
{
}


void
NetworkLogger::send_message(Logger::LogLevel level, struct timeval *t,
			    const char *component, bool is_exception,
			    const char *format, va_list va)
{
  struct timeval now;
  if ( t == NULL ) {
    gettimeofday(&now, NULL);
    t = &now;
  }

  char *tmp;
  if (vasprintf(&tmp, format, va) != -1) {
    llsf_log_msgs::LogMessage lm;
    lm.set_ts_sec(now.tv_sec);
    lm.set_ts_nsec(now.tv_usec * 1000);

    lm.set_component(component);
    lm.set_is_exception(is_exception);
    lm.set_message(tmp);
    switch (level) {
    case LL_DEBUG: lm.set_log_level(llsf_log_msgs::LogMessage::LL_DEBUG); break;
    case LL_INFO:  lm.set_log_level(llsf_log_msgs::LogMessage::LL_INFO); break;
    case LL_WARN:  lm.set_log_level(llsf_log_msgs::LogMessage::LL_WARN); break;
    case LL_ERROR: lm.set_log_level(llsf_log_msgs::LogMessage::LL_ERROR); break;
    default: lm.set_log_level(llsf_log_msgs::LogMessage::LL_INFO); break;
    }
    free(tmp);
    pb_server_->send_to_all(lm);
  }
}


void
NetworkLogger::send_message(Logger::LogLevel level, struct timeval *t,
			    const char *component, bool is_exception,
			    const char *message)
{
  struct timeval now;
  if ( t == NULL ) {
    gettimeofday(&now, NULL);
    t = &now;
  }

  llsf_log_msgs::LogMessage lm;
  lm.set_ts_sec(now.tv_sec);
  lm.set_ts_nsec(now.tv_usec * 1000);
  
  lm.set_component(component);
  lm.set_is_exception(is_exception);
  lm.set_message(message);
  switch (level) {
  case LL_DEBUG: lm.set_log_level(llsf_log_msgs::LogMessage::LL_DEBUG); break;
  case LL_INFO:  lm.set_log_level(llsf_log_msgs::LogMessage::LL_INFO); break;
  case LL_WARN:  lm.set_log_level(llsf_log_msgs::LogMessage::LL_WARN); break;
  case LL_ERROR: lm.set_log_level(llsf_log_msgs::LogMessage::LL_ERROR); break;
  default: lm.set_log_level(llsf_log_msgs::LogMessage::LL_INFO); break;
  }
  pb_server_->send_to_all(lm);
}


void
NetworkLogger::vlog_debug(const char *component, const char *format, va_list va)
{
  if (log_level <= LL_DEBUG) {
    send_message(LL_DEBUG, NULL, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::vlog_info(const char *component, const char *format, va_list va)
{
  if (log_level <= LL_INFO) {
    send_message(LL_INFO, NULL, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::vlog_warn(const char *component, const char *format, va_list va)
{
  if (log_level <= LL_WARN) {
    send_message(LL_WARN, NULL, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::vlog_error(const char *component, const char *format, va_list va)
{
  if (log_level <= LL_ERROR) {
    send_message(LL_ERROR, NULL, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::log_debug(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vlog_debug(component, format, arg);
  va_end(arg);
}


void
NetworkLogger::log_info(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vlog_info(component, format, arg);
  va_end(arg);
}


void
NetworkLogger::log_warn(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vlog_warn(component, format, arg);
  va_end(arg);
}


void
NetworkLogger::log_error(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vlog_error(component, format, arg);
  va_end(arg);
}


void
NetworkLogger::log_debug(const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_DEBUG) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_DEBUG, NULL, component, /* exception? */ true, *i);
    } 
  }
}

void
NetworkLogger::log_info(const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_INFO) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_INFO, NULL, component, /* exception? */ true, *i);
    } 
  }
}


void
NetworkLogger::log_warn(const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_WARN) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_WARN, NULL, component, /* exception? */ true, *i);
    } 
  }
}


void
NetworkLogger::log_error(const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_ERROR) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_ERROR, NULL, component, /* exception? */ true, *i);
    }
  }
}




void
NetworkLogger::vtlog_debug(struct timeval *t, const char *component,
			   const char *format, va_list va)
{
  if (log_level <= LL_DEBUG) {
    send_message(LL_DEBUG, t, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::vtlog_info(struct timeval *t, const char *component, const char *format, va_list va)
{
  if (log_level <= LL_INFO) {
    send_message(LL_INFO, t, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::vtlog_warn(struct timeval *t, const char *component, const char *format, va_list va)
{
  if (log_level <= LL_WARN) {
    send_message(LL_WARN, t, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::vtlog_error(struct timeval *t, const char *component, const char *format, va_list va)
{
  if (log_level <= LL_ERROR) {
    send_message(LL_ERROR, t, component, /* exception? */ false, format, va);
  }
}


void
NetworkLogger::tlog_debug(struct timeval *t, const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vtlog_debug(t, component, format, arg);
  va_end(arg);
}


void
NetworkLogger::tlog_info(struct timeval *t, const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vtlog_info(t, component, format, arg);
  va_end(arg);
}


void
NetworkLogger::tlog_warn(struct timeval *t, const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vtlog_warn(t, component, format, arg);
  va_end(arg);
}


void
NetworkLogger::tlog_error(struct timeval *t, const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  vtlog_error(t, component, format, arg);
  va_end(arg);
}


void
NetworkLogger::tlog_debug(struct timeval *t, const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_DEBUG) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_DEBUG, t, component, /* exception? */ true, *i);
    } 
  }
}

void
NetworkLogger::tlog_info(struct timeval *t, const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_INFO) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_INFO, t, component, /* exception? */ true, *i);
    } 
  }
}


void
NetworkLogger::tlog_warn(struct timeval *t, const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_WARN) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_WARN, t, component, /* exception? */ true, *i);
    } 
  }
}


void
NetworkLogger::tlog_error(struct timeval *t, const char *component, fawkes::Exception &e)
{
  if (log_level <= LL_ERROR) {
    for (fawkes::Exception::iterator i = e.begin(); i != e.end(); ++i) {
      send_message(LL_ERROR, t, component, /* exception? */ true, *i);
    }
  }
}

} // end namespace llsfrb
