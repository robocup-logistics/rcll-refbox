
/***************************************************************************
 *  network - Fawkes network logger
 *
 *  Created: Sat Dec 15 00:45:54 2007 (after I5 xmas party)
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

#ifndef __LOGGING_NETWORK_H_
#define __LOGGING_NETWORK_H_

#include <logging/logger.h>

namespace protobuf_comm {
  class ProtobufStreamServer;
}

namespace llsfrb {

class NetworkLogger : public Logger
{
 public:
  NetworkLogger(protobuf_comm::ProtobufStreamServer *server, LogLevel log_level = LL_DEBUG);
  virtual ~NetworkLogger();

  virtual void log_debug(const char *component, const char *format, ...);
  virtual void log_info(const char *component, const char *format, ...);
  virtual void log_warn(const char *component, const char *format, ...);
  virtual void log_error(const char *component, const char *format, ...);

  virtual void log_debug(const char *component, fawkes::Exception &e);
  virtual void log_info(const char *component, fawkes::Exception &e);
  virtual void log_warn(const char *component, fawkes::Exception &e);
  virtual void log_error(const char *component, fawkes::Exception &e);

  virtual void vlog_debug(const char *component, const char *format, va_list va);
  virtual void vlog_info(const char *component, const char *format, va_list va);
  virtual void vlog_warn(const char *component, const char *format, va_list va);
  virtual void vlog_error(const char *component, const char *format, va_list va);

  virtual void tlog_debug(struct timeval *t, const char *component, const char *format, ...);
  virtual void tlog_info(struct timeval *t, const char *component, const char *format, ...);
  virtual void tlog_warn(struct timeval *t, const char *component, const char *format, ...);
  virtual void tlog_error(struct timeval *t, const char *component, const char *format, ...);

  virtual void tlog_debug(struct timeval *t, const char *component, fawkes::Exception &e);
  virtual void tlog_info(struct timeval *t, const char *component, fawkes::Exception &e);
  virtual void tlog_warn(struct timeval *t, const char *component, fawkes::Exception &e);
  virtual void tlog_error(struct timeval *t, const char *component, fawkes::Exception &e);

  virtual void vtlog_debug(struct timeval *t, const char *component,
			   const char *format, va_list va);
  virtual void vtlog_info(struct timeval *t, const char *component,
			  const char *format, va_list va);
  virtual void vtlog_warn(struct timeval *t, const char *component,
			  const char *format, va_list va);
  virtual void vtlog_error(struct timeval *t, const char *component,
			   const char *format, va_list va);

 private:
  void send_message(Logger::LogLevel level, struct timeval *t,
		    const char *component, bool is_exception,
		    const char *format, va_list va);
  void send_message(Logger::LogLevel level, struct timeval *t,
		    const char *component, bool is_exception, const char *message);

  protobuf_comm::ProtobufStreamServer  *pb_server_;
};



} // end namespace llsfrb

#endif
