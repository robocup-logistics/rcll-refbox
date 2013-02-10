
/***************************************************************************
 *  message_register.h - Protobuf stream protocol - message register
 *
 *  Created: Fri Feb 01 15:43:36 2013
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

#ifndef __PROTOBUF_COMM_MESSAGE_REGISTER_H_
#define __PROTOBUF_COMM_MESSAGE_REGISTER_H_

#include <protobuf_comm/frame_header.h>

#include <type_traits>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <boost/utility.hpp>

#include <map>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <limits>

namespace protobuf_comm {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class MessageRegister : boost::noncopyable
{
 public:
  MessageRegister();
  ~MessageRegister();

  /** Add a new message type.
   * The template parameter must be a sub-class of google::protobuf::Message.
   * An instance is spawned and kept internally to spawn more on incoming messages.
   * @param component_id ID of component this message type belongs to
   * @param msg_type message type
   */
  template <class MT>
  typename std::enable_if<std::is_base_of<google::protobuf::Message, MT>::value, void>::type
  add_message_type(uint16_t component_id, uint16_t msg_type)
  {
    KeyType key(component_id, msg_type);
    if (message_types_.find(key) != message_types_.end()) {
      throw std::runtime_error("Message type already registered");
    }
    message_types_[key] = new MT();
  }

  /** Add a new message type.
   * The template parameter must be a sub-class of google::protobuf::Message.
   * An instance is spawned and kept internally to spawn more on incoming messages.
   */
  template <class MT>
  typename std::enable_if<std::is_base_of<google::protobuf::Message, MT>::value, void>::type
  add_message_type()
  {
    MT m;
    const google::protobuf::Descriptor *desc = m.GetDescriptor();
    const google::protobuf::EnumDescriptor *enumdesc = desc->FindEnumTypeByName("CompType");
    if (! enumdesc) {
      throw std::logic_error("Message does not have CompType enum");
    }
    const google::protobuf::EnumValueDescriptor *compdesc =
      enumdesc->FindValueByName("COMP_ID");
    const google::protobuf::EnumValueDescriptor *msgtdesc =
      enumdesc->FindValueByName("MSG_TYPE");
    if (! compdesc || ! msgtdesc) {
      throw std::logic_error("Message CompType enum hs no COMP_ID or MSG_TYPE value");
    }
    int comp_id = compdesc->number();
    int msg_type = msgtdesc->number();
    if (comp_id < 0 || comp_id > std::numeric_limits<uint16_t>::max()) {
      throw std::logic_error("Message has invalid COMP_ID");
    }
    if (msg_type < 0 || msg_type > std::numeric_limits<uint16_t>::max()) {
      throw std::logic_error("Message has invalid MSG_TYPE");
    }
    KeyType key(comp_id, msg_type);
    if (message_types_.find(key) != message_types_.end()) {
      throw std::runtime_error("Message type already registered");
    }
    message_types_[key] = new MT();
  }

  void remove_message_type(uint16_t component_id, uint16_t msg_type);

  std::shared_ptr<google::protobuf::Message>
  new_message_for(uint16_t component_id, uint16_t msg_type);

  void serialize(uint16_t component_id, uint16_t msg_type,
		 google::protobuf::Message &msg,
		 frame_header_t &frame_header, std::string &data);
  std::shared_ptr<google::protobuf::Message>
    deserialize(frame_header_t &frame_header, void *data);

 private:
  typedef std::pair<uint16_t, uint16_t> KeyType;
  typedef std::map<KeyType, google::protobuf::Message *> TypeMap;

  TypeMap message_types_;
};

} // end namespace protobuf_comm


#endif
