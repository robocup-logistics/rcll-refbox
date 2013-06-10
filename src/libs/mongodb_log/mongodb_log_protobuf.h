
/***************************************************************************
 *  mongodb_log_protobuf.h - MongoDB protobuf logger
 *
 *  Created: Fri Jun 07 16:33:35 2013
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

#ifndef __LIBS_MONGODB_LOG_MONGODB_LOG_PROTOBUF_H_
#define __LIBS_MONGODB_LOG_MONGODB_LOG_PROTOBUF_H_

#include <google/protobuf/message.h>
#include <string>
#include <mongo/bson/bson.h>


namespace fawkes {
  class Mutex;
}

namespace mongo {
  class DBClientBase;
}

class MongoDBLogProtobuf
{
 public:
  MongoDBLogProtobuf(std::string host_port, std::string collection);
  virtual ~MongoDBLogProtobuf();

  void write(google::protobuf::Message &m);
  void write(google::protobuf::Message &m, mongo::BSONObj &meta_data);

 private:
  void add_field(const ::google::protobuf::FieldDescriptor *field,
		 const ::google::protobuf::Message &m, mongo::BSONObjBuilder *b);
  void add_message(const google::protobuf::Message &m, mongo::BSONObjBuilder *b);

 private:
  std::string          collection_;
  fawkes::Mutex       *mutex_;
  mongo::DBClientBase *mongodb_;
};

#endif
