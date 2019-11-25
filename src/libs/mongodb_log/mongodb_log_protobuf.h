
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

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <mongocxx/client.hpp>
#include <string>

namespace fawkes {
  class Mutex;
}

class MongoDBLogProtobuf
{
public:
	MongoDBLogProtobuf(std::string host_port, std::string collection);
	virtual ~MongoDBLogProtobuf();

	void write(const google::protobuf::Message &m);
	void write(const google::protobuf::Message &m, const bsoncxx::document::view_or_value &meta_data);

private:
	void                              add_field(const ::google::protobuf::FieldDescriptor *field,
	                                            const ::google::protobuf::Message &        m,
	                                            bsoncxx::builder::basic::document *        doc);
	bsoncxx::builder::basic::document add_message(const google::protobuf::Message &m);

private:
	fawkes::Mutex *      mutex_;
	mongocxx::collection collection_;
};

#endif
