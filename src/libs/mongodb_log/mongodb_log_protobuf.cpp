
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

#include <mongodb_log/mongodb_log_protobuf.h>

#include <core/exception.h>
#include <core/threading/mutex.h>
#include <core/threading/mutex_locker.h>

#include <mongo/client/dbclient.h>
#include <google/protobuf/descriptor.h>

using namespace mongo;
using namespace google::protobuf;

/** @class MongoDBLogProtobuf <mongodb_log/mongodb_log_logger.h>
 * Thread that provides a logger writing to MongoDB.
 * This thread provides a logger, which writes log information to a
 * MongoDB collection.
 * @author Tim Niemueller
 */

/** Constructor. */
MongoDBLogProtobuf::MongoDBLogProtobuf(std::string host_port, std::string collection)
{
  mutex_ = new fawkes::Mutex();

  collection_ = collection;

  DBClientConnection *conn =
    new DBClientConnection(/* auto reconnect */ true);
  mongodb_ = conn;
  std::string errmsg;
  if (! conn->connect(host_port, errmsg)) {
    throw fawkes::Exception("Could not connect to MongoDB at %s: %s",
			    host_port.c_str(), errmsg.c_str());
  }
}


/** Destructor. */
MongoDBLogProtobuf::~MongoDBLogProtobuf()
{
  delete mutex_;
}

void
MongoDBLogProtobuf::add_field(const FieldDescriptor *field,
			      const ::google::protobuf::Message &m, BSONObjBuilder *b)
{
  const Reflection *refl = m.GetReflection();

  int count = 0;
  if (field->is_repeated()) {
    count = refl->FieldSize(m, field);
  } else if (refl->HasField(m, field)) {
    count = 1;
  }

  for (int j = 0; j < count; ++j) {
    switch (field->type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)		\
      case FieldDescriptor::TYPE_##TYPE: {				\
        const CPPTYPE value = field->is_repeated()			\
	  ? refl->GetRepeated##CPPTYPE_METHOD(m, field, j)		\
	  : refl->Get##CPPTYPE_METHOD(m, field);			\
	  b->append(field->name(), value);				\
	  break;							\
      }

      HANDLE_PRIMITIVE_TYPE( INT32, int,           Int32);
      HANDLE_PRIMITIVE_TYPE( INT64, long long int, Int64);
      HANDLE_PRIMITIVE_TYPE(SINT32, int,           Int32);
      HANDLE_PRIMITIVE_TYPE(SINT64, long long int, Int64);
      HANDLE_PRIMITIVE_TYPE(UINT32, unsigned int,  UInt32);
      HANDLE_PRIMITIVE_TYPE(UINT64, long long int, UInt64);

      HANDLE_PRIMITIVE_TYPE( FIXED32, unsigned int,  UInt32);
      HANDLE_PRIMITIVE_TYPE( FIXED64, long long int, UInt64);
      HANDLE_PRIMITIVE_TYPE(SFIXED32, int,           Int32);
      HANDLE_PRIMITIVE_TYPE(SFIXED64, long long int, Int64);

      HANDLE_PRIMITIVE_TYPE(FLOAT , float , Float );
      HANDLE_PRIMITIVE_TYPE(DOUBLE, double, Double);

      HANDLE_PRIMITIVE_TYPE(BOOL, bool, Bool);
#undef HANDLE_PRIMITIVE_TYPE

      case FieldDescriptor::TYPE_MESSAGE: {
	const google::protobuf::Message &sub_m = field->is_repeated()
	  ? refl->GetRepeatedMessage(m, field, j)
	  : refl->GetMessage(m, field);   

        BSONObjBuilder sub(b->subobjStart(field->name()));
	add_message(sub_m, &sub);
	sub.done();
        break;
      }

      case FieldDescriptor::TYPE_GROUP:
        break;

      case FieldDescriptor::TYPE_ENUM: {
        const EnumValueDescriptor* value = field->is_repeated()
	  ? refl->GetRepeatedEnum(m, field, j)
	  : refl->GetEnum(m, field);
	b->append(field->name(), value->name());
	break;
      }

      // Handle strings separately so that we can get string references
      // instead of copying.
      case FieldDescriptor::TYPE_STRING: {
        string scratch;
	const string& value = field->is_repeated()
	  ? refl->GetRepeatedStringReference(m, field, j, &scratch)
	  : refl->GetStringReference(m, field, &scratch);
	//VerifyUTF8String(value.data(), value.length(), SERIALIZE);
	b->append(field->name(), value);
	break;
      }
  
      case FieldDescriptor::TYPE_BYTES: {
        string scratch;
	const string& value = field->is_repeated()
	  ? refl->GetRepeatedStringReference(m, field, j, &scratch)
	  : refl->GetStringReference(m, field, &scratch);
	b->appendBinData(field->name(), value.size(), BinDataGeneral, value.c_str());
	break;
      }
    }
  }
}


void
MongoDBLogProtobuf::add_message(const google::protobuf::Message &m, BSONObjBuilder *b)
{
  b->append("_type", m.GetTypeName());

  std::string data;
  m.SerializeToString(&data);
  b->appendBinData("_protobuf", data.size(), BinDataGeneral, data.c_str());

  const Reflection *refl = m.GetReflection();

  std::vector<const FieldDescriptor *> fields;
  refl->ListFields(m, &fields);

  for (size_t i = 0; i < fields.size(); ++i) {
    add_field(fields[i], m, b);
  }
}

void
MongoDBLogProtobuf::write(google::protobuf::Message &m)
{
  fawkes::MutexLocker lock(mutex_);
  struct timeval now;
  gettimeofday(&now, NULL);
  Date_t nowd = now.tv_sec * 1000 + now.tv_usec / 1000;

  BSONObjBuilder b;
  b.appendDate("_time", nowd);

  add_message(m, &b);

  try {
    mongodb_->insert(collection_, b.obj());
  } catch (mongo::DBException &e) {} // ignored
}


void
MongoDBLogProtobuf::write(google::protobuf::Message &m, mongo::BSONObj &meta_data)
{
  fawkes::MutexLocker lock(mutex_);
  struct timeval now;
  gettimeofday(&now, NULL);
  Date_t nowd = now.tv_sec * 1000 + now.tv_usec / 1000;

  BSONObjBuilder b;
  b.appendDate("_time", nowd);

  add_message(m, &b);

  b.append("_meta", meta_data);

  try {
    mongodb_->insert(collection_, b.obj());
  } catch (mongo::DBException &e) {} // ignored
}
