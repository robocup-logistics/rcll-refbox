
/***************************************************************************
 *  refbox.cpp - LLSF RefBox main program
 *
 *  Created: Thu Feb 07 11:04:17 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "refbox.h"
#include "sps_comm.h"

#include <config/yaml.h>
#include <protobuf_comm/peer.h>

#include <google/protobuf/descriptor.h>
#include <boost/bind.hpp>

using namespace protobuf_comm;
using namespace google::protobuf;

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class LLSFRefBox "refbox.h"
 * LLSF referee box main application.
 * @author Tim Niemueller
 */ 

/** Constructor.
 * @param argc number of arguments passed
 * @param argv array of arguments
 */
LLSFRefBox::LLSFRefBox(int argc, char **argv)
  : timer_(io_service_)
{

  config_ = new YamlConfiguration(CONFDIR);
  config_->load("config.yaml");

  cfg_clips_dir_ = std::string(SRCDIR) + "/clips/";

  try {
    cfg_timer_interval_ = config_->get_uint("/llsfrb/clips/timer-interval");
  } catch (fawkes::Exception &e) {
    delete config_;
    throw;
  }

  try {
    printf("Connecting to SPS\n");
    sps_ = NULL;
    if (config_->get_bool("/llsfrb/sps/enable")) {
      sps_ = new SPSComm(config_->get_string("/llsfrb/sps/host").c_str(),
			 config_->get_uint("/llsfrb/sps/port"));

      sps_->reset_lights();
      sps_->reset_rfids();
      sps_->test_lights();
    }
  } catch (fawkes::Exception &e) {
    printf("Cannot connect to SPS, running without\n");
    delete sps_;
    sps_ = NULL;
  }

  setup_protobuf_comm();

  setup_clips();
}

/** Destructor. */
LLSFRefBox::~LLSFRefBox()
{
  timer_.cancel();
  delete pbc_server_;
  delete pbc_peer_;
  delete config_;
  delete sps_;
  delete clips_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}


void
LLSFRefBox::setup_protobuf_comm()
{
  try {
    pbc_server_ = NULL;
    pbc_peer_   = NULL;
    pbc_server_ = new ProtobufStreamServer(config_->get_uint("/llsfrb/comm/server-port"));
    pbc_peer_   = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/peer-host"),
					    config_->get_uint("/llsfrb/comm/peer-port"));

    pbc_server_->signal_connected()
      .connect(boost::bind(&LLSFRefBox::handle_client_connected, this, _1, _2));
    pbc_server_->signal_disconnected()
      .connect(boost::bind(&LLSFRefBox::handle_client_disconnected, this, _1, _2));
    pbc_server_->signal_received()
      .connect(boost::bind(&LLSFRefBox::handle_client_msg, this, _1, _2, _3, _4));
    pbc_peer_->signal_received()
      .connect(boost::bind(&LLSFRefBox::handle_peer_msg, this, _1, _2, _3, _4));
  } catch (std::runtime_error &e) {
    delete config_;
    delete sps_;
    delete pbc_server_;
    delete pbc_peer_;
    throw;
  }


  //MessageRegister &mr = pbc_server_->message_register();
  //mr.add_message_type<...>(1, 2);
}

void
LLSFRefBox::setup_clips()
{
  printf("Creating CLIPS environment\n");
  clips_ = new CLIPS::Environment();

  clips_->add_function("get-clips-dirs", sigc::slot<CLIPS::Values>(sigc::mem_fun(*this, &LLSFRefBox::clips_get_clips_dirs)));
  clips_->add_function("now", sigc::slot<CLIPS::Values>(sigc::mem_fun(*this, &LLSFRefBox::clips_now)));
  clips_->add_function("load-config", sigc::slot<void, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_load_config)));
  clips_->add_function("pb-field-names", sigc::slot<CLIPS::Values, void *>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_field_names)));
  clips_->add_function("pb-field-type", sigc::slot<CLIPS::Value, void *, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_field_type)));
  clips_->add_function("pb-field-label", sigc::slot<CLIPS::Value, void *, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_field_label)));
  clips_->add_function("pb-field-value", sigc::slot<CLIPS::Value, void *, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_field_value)));

  clips_->signal_periodic().connect(sigc::mem_fun(*this, &LLSFRefBox::handle_clips_periodic));

  if (!clips_->batch_evaluate(cfg_clips_dir_ + "init.clp")) {
    printf("Failed to initialize CLIPS environment, batch file failed.");
    throw fawkes::Exception("Failed to initialize CLIPS environment, batch file failed.");
  }  

  clips_->assert_fact("(init)");
  clips_->refresh_agenda();
  clips_->run();
}


void
LLSFRefBox::clips_assert_message(std::pair<std::string, unsigned short> &endpoint,
				 uint16_t comp_id, uint16_t msg_type,
				 std::shared_ptr<google::protobuf::Message> &msg,
				 unsigned int client_id)
{
  CLIPS::Template::pointer temp = clips_->get_template("protobuf-msg");
  if (temp) {
    void *ptr = new std::shared_ptr<google::protobuf::Message>(msg);
    CLIPS::Fact::pointer fact = CLIPS::Fact::create(*clips_, temp);
    fact->set_slot("type", msg->GetTypeName());
    fact->set_slot("comp-id", comp_id);
    fact->set_slot("msg-type", msg_type);
    fact->set_slot("rcvd-via", CLIPS::Value((client_id == 0) ? "BROADCAST" : "STREAM"));
    CLIPS::Values host_port(2, CLIPS::Value(CLIPS::TYPE_STRING));
    host_port[0] = endpoint.first;
    host_port[1] = CLIPS::Value(endpoint.second);
    fact->set_slot("rcvd-from", host_port);
    fact->set_slot("ptr", CLIPS::Value(ptr));
    CLIPS::Fact::pointer new_fact = clips_->assert_fact(fact);

    if (new_fact) {
      clips_msg_facts_[new_fact->index()] = new_fact;
    } else {
      printf("Asserting protobuf-msg fact failed\n");
      delete static_cast<std::shared_ptr<google::protobuf::Message> *>(ptr);
    }
  } else {
    printf("Did not get template, did you load protobuf.clp?\n");
  }
}

void
LLSFRefBox::handle_clips_periodic()
{
  std::lock_guard<std::recursive_mutex> lock(clips_mutex_);

  std::queue<int> to_erase;
  std::map<long int, CLIPS::Fact::pointer>::iterator f;
  for (f = clips_msg_facts_.begin(); f != clips_msg_facts_.end(); ++f) {
    CLIPS::Template::pointer temp = f->second->get_template();
    if (f->second->refcount() == 1) {
      printf("Fact %li can be erased\n", f->second->index());
      to_erase.push(f->first);
    }
  }
  while (! to_erase.empty()) {
    long int index = to_erase.front();
    CLIPS::Fact::pointer &f = clips_msg_facts_[index];
    CLIPS::Value v = f->slot_value("ptr")[0];
    void *ptr = v.as_address();
    delete static_cast<std::shared_ptr<google::protobuf::Message> *>(ptr);
    clips_msg_facts_.erase(index);
    to_erase.pop();
  }
}


CLIPS::Values
LLSFRefBox::clips_pb_field_names(void *msgptr)
{
  google::protobuf::Message *m = static_cast<google::protobuf::Message *>(msgptr);

  const Descriptor *desc       = m->GetDescriptor();
  const int field_count = desc->field_count();
  CLIPS::Values field_names(field_count);
  for (int i = 0; i < field_count; ++i) {
    field_names[i].set(desc->field(i)->name(), true);
  }
  return field_names;
}

CLIPS::Value
LLSFRefBox::clips_pb_field_type(void *msgptr, std::string field_name)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  switch (field->type()) {
  case FieldDescriptor::TYPE_DOUBLE:   return CLIPS::Value("DOUBLE", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_FLOAT:    return CLIPS::Value("FLOAT", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_INT64:    return CLIPS::Value("INT64", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_UINT64:   return CLIPS::Value("UINT64", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_INT32:    return CLIPS::Value("INT32", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_FIXED64:  return CLIPS::Value("FIXED64", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_FIXED32:  return CLIPS::Value("FIXED32", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_BOOL:     return CLIPS::Value("BOOL", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_STRING:   return CLIPS::Value("STRING", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_MESSAGE:  return CLIPS::Value("MESSAGE", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_BYTES:    return CLIPS::Value("BYTES", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_UINT32:   return CLIPS::Value("UINT32", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_ENUM:     return CLIPS::Value("ENUM", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_SFIXED32: return CLIPS::Value("SFIXED32", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_SFIXED64: return CLIPS::Value("SFIXED64", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_SINT32:   return CLIPS::Value("SINT32", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_SINT64:   return CLIPS::Value("SINT64", CLIPS::TYPE_SYMBOL);
  default: return CLIPS::Value("UNKNOWN", CLIPS::TYPE_SYMBOL);
  }
}

CLIPS::Value
LLSFRefBox::clips_pb_field_label(void *msgptr, std::string field_name)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  switch (field->label()) {
  case FieldDescriptor::LABEL_OPTIONAL: return CLIPS::Value("OPTIONAL", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::LABEL_REQUIRED: return CLIPS::Value("REQUIRED", CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::LABEL_REPEATED: return CLIPS::Value("REPEATED", CLIPS::TYPE_SYMBOL);
  default:                              return CLIPS::Value("UNKNOWN", CLIPS::TYPE_SYMBOL);
  }
}

CLIPS::Value
LLSFRefBox::clips_pb_field_value(void *msgptr, std::string field_name)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  const Reflection *refl       = (*m)->GetReflection();
  switch (field->type()) {
  case FieldDescriptor::TYPE_DOUBLE:   return CLIPS::Value(refl->GetDouble(**m, field));
  case FieldDescriptor::TYPE_FLOAT:    return CLIPS::Value(refl->GetFloat(**m, field));
  case FieldDescriptor::TYPE_INT64:    return CLIPS::Value(refl->GetInt64(**m, field));
  case FieldDescriptor::TYPE_UINT64:
    return CLIPS::Value((long int)refl->GetUInt64(**m, field));
  case FieldDescriptor::TYPE_INT32:    return CLIPS::Value(refl->GetInt32(**m, field));
  case FieldDescriptor::TYPE_FIXED64:
    return CLIPS::Value((long int)refl->GetUInt64(**m, field));
  case FieldDescriptor::TYPE_FIXED32:  return CLIPS::Value(refl->GetUInt32(**m, field));
  case FieldDescriptor::TYPE_BOOL:     return CLIPS::Value(refl->GetBool(**m, field));
  case FieldDescriptor::TYPE_STRING:   return CLIPS::Value(refl->GetString(**m, field));
  case FieldDescriptor::TYPE_MESSAGE:
    {
      const google::protobuf::Message &mfield = refl->GetMessage(**m, field);
      google::protobuf::Message *mcopy = mfield.New();
      mcopy->CopyFrom(mfield);
      void *ptr = new std::shared_ptr<google::protobuf::Message>(mcopy);
      return CLIPS::Value(ptr);
    }
  case FieldDescriptor::TYPE_BYTES:    return CLIPS::Value((char *)"bytes");
  case FieldDescriptor::TYPE_UINT32:   return CLIPS::Value(refl->GetUInt32(**m, field));
  case FieldDescriptor::TYPE_ENUM:
    return CLIPS::Value(refl->GetEnum(**m, field)->name(), CLIPS::TYPE_SYMBOL);
  case FieldDescriptor::TYPE_SFIXED32: return CLIPS::Value(refl->GetInt32(**m, field));
  case FieldDescriptor::TYPE_SFIXED64: return CLIPS::Value(refl->GetInt64(**m, field));
  case FieldDescriptor::TYPE_SINT32:   return CLIPS::Value(refl->GetInt32(**m, field));
  case FieldDescriptor::TYPE_SINT64:   return CLIPS::Value(refl->GetInt64(**m, field));
  default:
    throw std::logic_error("Unknown protobuf field type encountered");
  }
}

CLIPS::Values
LLSFRefBox::clips_now()
{
  CLIPS::Values rv;
  struct timeval tv;
  gettimeofday(&tv, 0);
  rv.push_back(tv.tv_sec);
  rv.push_back(tv.tv_usec);
  return rv;
}


CLIPS::Values
LLSFRefBox::clips_get_clips_dirs()
{
  CLIPS::Values rv;
  rv.push_back(cfg_clips_dir_);
  return rv;
}

void
LLSFRefBox::clips_load_config(std::string cfg_prefix)
{
  std::auto_ptr<Configuration::ValueIterator> v(config_->search(cfg_prefix.c_str()));
  while (v->next()) {
    std::string type = "";
    std::string value = v->get_as_string();

    if (v->is_float())       type = "FLOAT";
    else if (v->is_uint())   type = "UINT";
    else if (v->is_int())    type = "INT";
    else if (v->is_bool())   type = "BOOL";
    else if (v->is_string()) {
      type = "STRING";
      value = std::string("\"") + value + "\"";
    } else {
      printf("Config value at '%s' of unknown type '%s'",
	     v->path(), v->type());
    }

    //logger->log_info(name(), "ASSERT (confval (path \"%s\") (type %s) (value %s)",
    //		     v->path(), type.c_str(), v->get_as_string().c_str());
    clips_->assert_fact_f("(confval (path \"%s\") (type %s) (value %s))",
			 v->path(), type.c_str(), value.c_str());
  }
}



/** Start the timer for another run. */
void
LLSFRefBox::start_timer()
{
  timer_last_ = boost::posix_time::microsec_clock::local_time();
  timer_.expires_from_now(boost::posix_time::milliseconds(cfg_timer_interval_));
  timer_.async_wait(boost::bind(&LLSFRefBox::handle_timer, this,
				boost::asio::placeholders::error));
}

/** Handle timer event.
 * @param error error code
 */
void
LLSFRefBox::handle_timer(const boost::system::error_code& error)
{
  if (! error) {
    /*
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    long ms = (now - timer_last_).total_milliseconds();
    timer_last_ = now;
    */
    std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
    clips_->refresh_agenda();
    clips_->run();

    timer_.expires_at(timer_.expires_at()
		      + boost::posix_time::milliseconds(cfg_timer_interval_));
    timer_.async_wait(boost::bind(&LLSFRefBox::handle_timer, this,
				  boost::asio::placeholders::error));
  }
}


/** Handle operating system signal.
 * @param error error code
 * @param signum signal number
 */
void
LLSFRefBox::handle_signal(const boost::system::error_code& error, int signum)
{
  timer_.cancel();
}


void
LLSFRefBox::handle_client_connected(ProtobufStreamServer::ClientID client,
				    boost::asio::ip::tcp::endpoint &endpoint)
{
  std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
  client_endpoints_[client] = std::make_pair(endpoint.address().to_string(), endpoint.port());
  clips_->assert_fact_f("(protobuf-client-connected %u %s %u)", client,
			endpoint.address().to_string().c_str(), endpoint.port());
  clips_->refresh_agenda();
  clips_->run();
}


void
LLSFRefBox::handle_client_disconnected(ProtobufStreamServer::ClientID client,
				       const boost::system::error_code &error)
{
  std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
  clips_->assert_fact_f("(protobuf-client-disconnected %u)", client);
  clips_->refresh_agenda();
  clips_->run();
}


/** Handle message that came from a client.
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message
 */
void
LLSFRefBox::handle_client_msg(ProtobufStreamServer::ClientID client,
			      uint16_t component_id, uint16_t msg_type,
			      std::shared_ptr<google::protobuf::Message> msg)
{
  clips_assert_message(client_endpoints_[client],
		       component_id, msg_type, msg, client);
}


/** Handle message that came from a peer/robot
 * @param endpoint the endpoint from which the message was received
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message
 */
void
LLSFRefBox::handle_peer_msg(boost::asio::ip::udp::endpoint &endpoint,
			    uint16_t component_id, uint16_t msg_type,
			    std::shared_ptr<google::protobuf::Message> msg)
{
  std::pair<std::string, unsigned short> endpp =
    std::make_pair(endpoint.address().to_string(), endpoint.port());
  clips_assert_message(endpp, component_id, msg_type, msg);
}


/** Run the application.
 * @return return code, 0 if no error, error code otherwise
 */
int
LLSFRefBox::run()
{
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(boost::bind(&LLSFRefBox::handle_signal, this,
				 boost::asio::placeholders::error,
				 boost::asio::placeholders::signal_number));

  start_timer();
  io_service_.run();
  return 0;
}

} // end of namespace llsfrb
