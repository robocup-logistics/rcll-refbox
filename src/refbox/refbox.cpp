
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

#include <config/yaml.h>
#include <protobuf_comm/peer.h>
#include <msgs/BeaconSignal.pb.h>
#include <llsf_sps/sps_comm.h>

#include <google/protobuf/descriptor.h>
#include <boost/bind.hpp>

using namespace llsf_sps;
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
      bool test_lights = true;
      try {
	test_lights = config_->get_bool("/llsfrb/sps/test-lights");
      } catch (fawkes::Exception &e) {} // ignore, use default

      sps_ = new SPSComm(config_->get_string("/llsfrb/sps/host").c_str(),
			 config_->get_uint("/llsfrb/sps/port"));

      sps_->reset_lights();
      sps_->reset_rfids();
      if (test_lights) {
	sps_->test_lights();
      }
    }
  } catch (fawkes::Exception &e) {
    printf("Cannot connect to SPS, running without\n");
    delete sps_;
    sps_ = NULL;
  }

  setup_clips();

  setup_protobuf_comm();
}

/** Destructor. */
LLSFRefBox::~LLSFRefBox()
{
  timer_.cancel();

  std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
  clips_->assert_fact("(finalize)");
  clips_->refresh_agenda();
  clips_->run();

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

    if (config_->exists("/llsfrb/comm/peer-send-port") &&
	config_->exists("/llsfrb/comm/peer-recv-port") )
    {
      pbc_peer_   = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/peer-host"),
					      config_->get_uint("/llsfrb/comm/peer-send-port"),
					      config_->get_uint("/llsfrb/comm/peer-recv-port"));
    } else {
      pbc_peer_   = new ProtobufBroadcastPeer(config_->get_string("/llsfrb/comm/peer-host"),
					      config_->get_uint("/llsfrb/comm/peer-port"));
    }

    pbc_server_->signal_connected()
      .connect(boost::bind(&LLSFRefBox::handle_client_connected, this, _1, _2));
    pbc_server_->signal_disconnected()
      .connect(boost::bind(&LLSFRefBox::handle_client_disconnected, this, _1, _2));
    pbc_server_->signal_received()
      .connect(boost::bind(&LLSFRefBox::handle_client_msg, this, _1, _2, _3, _4));
    pbc_peer_->signal_received()
      .connect(boost::bind(&LLSFRefBox::handle_peer_msg, this, _1, _2, _3, _4));
    pbc_peer_->signal_error()
      .connect(boost::bind(&LLSFRefBox::handle_peer_error, this, _1));
  } catch (std::runtime_error &e) {
    delete config_;
    delete sps_;
    delete pbc_server_;
    delete pbc_peer_;
    throw;
  }


  MessageRegister &mr = pbc_server_->message_register();
  mr.add_message_type<llsf_msgs::BeaconSignal>();
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
  clips_->add_function("pb-field-list", sigc::slot<CLIPS::Values, void *, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_field_list)));
  clips_->add_function("pb-field-is-list", sigc::slot<bool, void *, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_field_is_list)));
  clips_->add_function("pb-create", sigc::slot<CLIPS::Value, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_create)));
  clips_->add_function("pb-destroy", sigc::slot<void, void *>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_destroy)));
  clips_->add_function("pb-ref", sigc::slot<CLIPS::Value, void *>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_ref)));
  clips_->add_function("pb-set-field", sigc::slot<void, void *, std::string, CLIPS::Value>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_set_field)));
  clips_->add_function("pb-send", sigc::slot<void, long int, void *>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_send)));
  clips_->add_function("pb-broadcast", sigc::slot<void, void *>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_broadcast)));
  clips_->add_function("pb-disconnect-client", sigc::slot<void, long int>(sigc::mem_fun(*this, &LLSFRefBox::clips_pb_disconnect)));
  clips_->add_function("sps-set-signal", sigc::slot<void, std::string, std::string, std::string>(sigc::mem_fun(*this, &LLSFRefBox::clips_sps_set_signal)));

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
  std::lock_guard<std::recursive_mutex> lock(clips_mutex_);

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
    fact->set_slot("client-id", client_id);
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


CLIPS::Value
LLSFRefBox::clips_pb_create(std::string full_name)
{
  try {
    std::shared_ptr<google::protobuf::Message> m =
      pbc_server_->message_register().new_message_for(full_name);
    return CLIPS::Value(new std::shared_ptr<google::protobuf::Message>(m));
  } catch (std::runtime_error &e) {
    printf("Cannot create message of type %s: %s\n", full_name.c_str(), e.what());
    return CLIPS::Value(new std::shared_ptr<google::protobuf::Message>());
  }
}


CLIPS::Value
LLSFRefBox::clips_pb_ref(void *msgptr)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) return new std::shared_ptr<google::protobuf::Message>();

  return CLIPS::Value(new std::shared_ptr<google::protobuf::Message>(*m));
}


void
LLSFRefBox::clips_pb_destroy(void *msgptr)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) return;

  delete m;
}


CLIPS::Values
LLSFRefBox::clips_pb_field_names(void *msgptr)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) return CLIPS::Values();

  const Descriptor *desc = (*m)->GetDescriptor();
  const int field_count  = desc->field_count();
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
  if (!*m) return CLIPS::Value("INVALID-MESSAGE", CLIPS::TYPE_SYMBOL);

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  if (! field) {
    return CLIPS::Value("DOES-NOT-EXIST", CLIPS::TYPE_SYMBOL);
  }
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
  if (!*m) return CLIPS::Value("INVALID-MESSAGE", CLIPS::TYPE_SYMBOL);

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  if (! field) {
    return CLIPS::Value("DOES-NOT-EXIST", CLIPS::TYPE_SYMBOL);
  }
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
  if (!*m) return CLIPS::Value("INVALID-MESSAGE", CLIPS::TYPE_SYMBOL);

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  if (! field) {
    printf("Field %s of %s does not exist\n",
	   field_name.c_str(), (*m)->GetTypeName().c_str());
    return CLIPS::Value("DOES-NOT-EXIST", CLIPS::TYPE_SYMBOL);
  }
  const Reflection *refl       = (*m)->GetReflection();
  if (field->type() != FieldDescriptor::TYPE_MESSAGE && ! refl->HasField(**m, field)) {
    printf("Field %s of %s not set\n",
	   field_name.c_str(), (*m)->GetTypeName().c_str());
    return CLIPS::Value("NOT-SET", CLIPS::TYPE_SYMBOL);
  }
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


void
LLSFRefBox::clips_pb_set_field(void *msgptr, std::string field_name, CLIPS::Value value)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) return;

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  if (! field) {
    printf("Could not find field %s\n", field_name.c_str());
    return;
  }
  const Reflection *refl       = (*m)->GetReflection();

  switch (field->type()) {
  case FieldDescriptor::TYPE_DOUBLE:   refl->SetDouble(m->get(), field, value); break;
  case FieldDescriptor::TYPE_FLOAT:    refl->SetFloat(m->get(), field, value);  break;
  case FieldDescriptor::TYPE_SFIXED64:
  case FieldDescriptor::TYPE_SINT64:
  case FieldDescriptor::TYPE_INT64:
    refl->SetInt64(m->get(), field, value);  break;
  case FieldDescriptor::TYPE_FIXED64:
  case FieldDescriptor::TYPE_UINT64:
    refl->SetUInt64(m->get(), field, (long int)value); break;
  case FieldDescriptor::TYPE_SFIXED32:
  case FieldDescriptor::TYPE_SINT32:
  case FieldDescriptor::TYPE_INT32:
    refl->SetInt32(m->get(), field, value); break;
  case FieldDescriptor::TYPE_BOOL:
    refl->SetBool(m->get(), field, (value == "TRUE"));
    break;
  case FieldDescriptor::TYPE_STRING:   refl->SetString(m->get(), field, value); break;
  case FieldDescriptor::TYPE_MESSAGE:
    {
      std::shared_ptr<google::protobuf::Message> *mfrom =
	static_cast<std::shared_ptr<google::protobuf::Message> *>(value.as_address());
      Message *mut_msg = refl->MutableMessage(m->get(), field);
      mut_msg->CopyFrom(**mfrom);
      delete mfrom;
    }
    break;
  case FieldDescriptor::TYPE_BYTES:    break;
  case FieldDescriptor::TYPE_FIXED32:
  case FieldDescriptor::TYPE_UINT32:
    refl->SetUInt32(m->get(), field, value); break;
  case FieldDescriptor::TYPE_ENUM:
    {
      const EnumDescriptor *enumdesc = field->enum_type();
      const EnumValueDescriptor *enumval = enumdesc->FindValueByName(value);
      if (enumval)  refl->SetEnum(m->get(), field, enumval);
    }
    break;
  default:
    throw std::logic_error("Unknown protobuf field type encountered");
  }
}


void
LLSFRefBox::clips_pb_send(long int client_id, void *msgptr)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) {
    printf("Cannot send to %li: invalid message\n", client_id);
    return;
  }

  //printf("Sending to %li\n", client_id);
  if (client_id > 0 && client_id < std::numeric_limits<unsigned int>::max()) {
    try {
      pbc_server_->send(client_id, *m);
    } catch (google::protobuf::FatalException &e) {
      printf("Failed to send message of type %s: %s\n",
	     (*m)->GetTypeName().c_str(), e.what());
    }
  }
}


void
LLSFRefBox::clips_pb_broadcast(void *msgptr)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) {
    printf("Cannot send broadcast: invalid message\n");
    return;
  }

  //printf("Broadcasting %s\n", (*m)->GetTypeName().c_str());
  try {
    pbc_peer_->send(*m);
  } catch (google::protobuf::FatalException &e) {
    printf("Failed to broadcast message of type %s: %s\n",
	   (*m)->GetTypeName().c_str(), e.what());
  }
}


void
LLSFRefBox::clips_pb_disconnect(long int client_id)
{
  printf("Disconnecting client %li\n", client_id);

  try {
    pbc_server_->disconnect(client_id);
  } catch (std::runtime_error &e) {
    printf("Failed to disconnect from client %li: %s", client_id, e.what());
  }
}

CLIPS::Values
LLSFRefBox::clips_pb_field_list(void *msgptr, std::string field_name)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) return CLIPS::Values(1, CLIPS::Value("INVALID-MESSAGE", CLIPS::TYPE_SYMBOL));

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  if (! field) {
    return CLIPS::Values(1, CLIPS::Value("DOES-NOT-EXIST", CLIPS::TYPE_SYMBOL));
  }
  if (field->label() == FieldDescriptor::LABEL_REQUIRED ||
      field->label() == FieldDescriptor::LABEL_OPTIONAL)
  {
    CLIPS::Values rv(1, clips_pb_field_value(msgptr, field_name));
    return rv;
  }

  const Reflection *refl       = (*m)->GetReflection();
  int field_size = refl->FieldSize(**m, field);
  CLIPS::Values rv(field_size);
  for (int i = 0; i < field_size; ++i) {
    switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
      rv[i] = CLIPS::Value(refl->GetRepeatedDouble(**m, field, i));
      break;
    case FieldDescriptor::TYPE_FLOAT:
      rv[i] = CLIPS::Value(refl->GetRepeatedFloat(**m, field, i));
      break;
      break;
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
      rv[i] = CLIPS::Value((long int)refl->GetRepeatedUInt64(**m, field, i));
      break;
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
      rv[i] = CLIPS::Value(refl->GetRepeatedUInt32(**m, field, i));
      break;
    case FieldDescriptor::TYPE_BOOL:
      rv[i] = CLIPS::Value(refl->GetRepeatedBool(**m, field, i));
      break;
    case FieldDescriptor::TYPE_STRING:
      rv[i] = CLIPS::Value(refl->GetRepeatedString(**m, field, i));
      break;
    case FieldDescriptor::TYPE_MESSAGE:
      {
	google::protobuf::Message *mcopy = (*m)->New();
	mcopy->CopyFrom(refl->GetRepeatedMessage(**m, field, i));
	void *ptr = new std::shared_ptr<google::protobuf::Message>(mcopy);
	rv[i] = CLIPS::Value(ptr);
      }
    case FieldDescriptor::TYPE_BYTES:
      rv[i] = CLIPS::Value((char *)"BYTES", CLIPS::TYPE_SYMBOL);
      break;
    case FieldDescriptor::TYPE_ENUM:
      rv[i] = CLIPS::Value(refl->GetRepeatedEnum(**m, field, i)->name(), CLIPS::TYPE_SYMBOL);
      break;
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
      rv[i] = CLIPS::Value(refl->GetRepeatedInt32(**m, field, i));
      break;
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64:
      rv[i] = CLIPS::Value(refl->GetRepeatedInt64(**m, field, i));
      break;
    default:
      throw std::logic_error("Unknown protobuf field type encountered");
    }
  }

  return rv;
}


bool
LLSFRefBox::clips_pb_field_is_list(void *msgptr, std::string field_name)
{
  std::shared_ptr<google::protobuf::Message> *m =
    static_cast<std::shared_ptr<google::protobuf::Message> *>(msgptr);
  if (!*m) return false;

  const Descriptor *desc       = (*m)->GetDescriptor();
  const FieldDescriptor *field = desc->FindFieldByName(field_name);
  if (! field) {
    return false;
  }
  return (field->label() == FieldDescriptor::LABEL_REPEATED);
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
      if (! v->is_list()) {
	value = std::string("\"") + value + "\"";
      }
    } else {
      printf("Config value at '%s' of unknown type '%s'",
	     v->path(), v->type());
    }

    if (v->is_list()) {
      //printf("(confval (path \"%s\") (type %s) (is-list TRUE) (list-value %s))\n",
      //       v->path(), type.c_str(), value.c_str());
      clips_->assert_fact_f("(confval (path \"%s\") (type %s) (is-list TRUE) (list-value %s))",
			    v->path(), type.c_str(), value.c_str());
    } else {
      //printf("(confval (path \"%s\") (type %s) (value %s))\n",
      //       v->path(), type.c_str(), value.c_str());
      clips_->assert_fact_f("(confval (path \"%s\") (type %s) (value %s))",
			    v->path(), type.c_str(), value.c_str());
    }
  }
}


void
LLSFRefBox::clips_sps_set_signal(std::string machine, std::string light, std::string state)
{
  if (! sps_)  return;
  try {
    sps_->set_light(machine, light, state);
  } catch (fawkes::Exception &e) {
    printf("Failed to set signal: %s\n", e.what());
  }
}


void
LLSFRefBox::sps_read_rfids()
{
  if (! sps_)  return;

  std::lock_guard<std::recursive_mutex> lock(clips_mutex_);

  std::vector<uint32_t> puck_ids = sps_->read_rfids();
  for (unsigned int i = 0; i < puck_ids.size(); ++i) {
    std::string & machine_name = sps_->index_to_name(i);
    if (puck_ids[i] == SPSComm::NO_PUCK) {
      clips_->assert_fact_f("(rfid-input (machine %s) (has-puck FALSE))",
			    machine_name.c_str());
    } else {
      clips_->assert_fact_f("(rfid-input (machine %s) (has-puck TRUE) (id %u))",
			    machine_name.c_str(), puck_ids[i]);
    }
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

    sps_read_rfids();

    std::lock_guard<std::recursive_mutex> lock(clips_mutex_);
    clips_->assert_fact("(time (now))");
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


/** Handle error during peer message processing.
 * @param error the error that happened
 */
void
LLSFRefBox::handle_peer_error(const boost::system::error_code &error)
{
  printf("Failed to receive peer message: %s\n", error.message().c_str());
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
