
/***************************************************************************
 *  mongodb_log_logger.h - MongoDB text logger
 *
 *  Created: Fri Jun 07 16:07:26 2013
 *           (based on Fawkes' mongodb_log_logger_thread.h)
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

#ifndef __LIBS_MONGODB_LOG_MONGODB_LOG_LOGGER_H_
#define __LIBS_MONGODB_LOG_MONGODB_LOG_LOGGER_H_

#include <core/exception.h>
#include <logging/logger.h>
#include <string>

namespace fawkes {
  class Mutex;
}

namespace mongo {
  class DBClientBase;
}

class MongoDBLogLogger : public llsfrb::Logger
{
 public:
  MongoDBLogLogger(std::string host_port, std::string collection);
  virtual ~MongoDBLogLogger();

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

  virtual void tlog_debug(struct timeval *t, const char *component,
			  const char *format, ...);
  virtual void tlog_info(struct timeval *t, const char *component,
			 const char *format, ...);
  virtual void tlog_warn(struct timeval *t, const char *component,
			 const char *format, ...);
  virtual void tlog_error(struct timeval *t, const char *component,
			  const char *format, ...);

  virtual void tlog_debug(struct timeval *t, const char *component,
			  fawkes::Exception &e);
  virtual void tlog_info(struct timeval *t, const char *component,
			 fawkes::Exception &e);
  virtual void tlog_warn(struct timeval *t, const char *component,
			 fawkes::Exception &e);
  virtual void tlog_error(struct timeval *t, const char *component,
			  fawkes::Exception &e);

  virtual void vtlog_debug(struct timeval *t, const char *component,
			   const char *format, va_list va);
  virtual void vtlog_info(struct timeval *t, const char *component,
			  const char *format, va_list va);
  virtual void vtlog_warn(struct timeval *t, const char *component,
			  const char *format, va_list va);
  virtual void vtlog_error(struct timeval *t, const char *component,
			   const char *format, va_list va);

 private:
  void insert_message(LogLevel ll, const char *component, const char *format,
		      va_list va);
  void insert_message(LogLevel ll, const char *component, fawkes::Exception &e);
  void tlog_insert_message(LogLevel ll, struct timeval *t, const char *component,
			   const char *format, va_list va);
  void tlog_insert_message(LogLevel ll, struct timeval *t, const char *component,
			   fawkes::Exception &);

 private:
  std::string          collection_;
  fawkes::Mutex       *mutex_;
  mongo::DBClientBase *mongodb_;
};

#endif
