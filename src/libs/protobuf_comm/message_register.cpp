
/***************************************************************************
 *  message_register.cpp - Protobuf stream protocol - message register
 *
 *  Created: Fri Feb 01 15:48:36 2013
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

#include <protobuf_comm/message_register.h>

#include <netinet/in.h>

namespace protobuf_comm {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


/** @class MessageRegister <protobuf_comm/message_register.h>
 * Register to map msg type numbers to Protobuf messages.
 * The register is used to automatically parse incoming messages to the
 * appropriate type. In your application, you need to register any
 * message you want to read. All unknown messages are silently dropped.
 * @author Tim Niemueller
 */

/** Constructor. */
MessageRegister::MessageRegister()
{
}

/** Destructor. */
MessageRegister::~MessageRegister()
{
  TypeMap::iterator m;
  for (m = message_by_comp_type_.begin(); m != message_by_comp_type_.end(); ++m) {
    delete m->second;
  }
}

/** Remove the given message type.
 * @param component_id ID of component this message type belongs to
 * @param msg_type message type
 */
void
MessageRegister::remove_message_type(uint16_t component_id, uint16_t msg_type)
{
  KeyType key(component_id, msg_type);
  if (message_by_comp_type_.find(key) != message_by_comp_type_.end()) {
    message_by_typename_.erase(message_by_comp_type_[key]->GetDescriptor()->full_name());
    message_by_comp_type_.erase(key);
  }
}

/** Create a new message instance.
 * @param component_id ID of component this message type belongs to
 * @param msg_type message type
 * @return new instance of a protobuf message that has been registered
 * for the given message type.
 */
std::shared_ptr<google::protobuf::Message>
MessageRegister::new_message_for(uint16_t component_id, uint16_t msg_type)
{
  KeyType key(component_id, msg_type);
  if (message_by_comp_type_.find(key) == message_by_comp_type_.end()) {
    std::string msg = "Message type " + std::to_string(component_id) + ":" +
      std::to_string(msg_type) + " not registered";
    throw std::runtime_error(msg);
  }

  google::protobuf::Message *m = message_by_comp_type_[key]->New();
  return std::shared_ptr<google::protobuf::Message>(m);
}


/** Create a new message instance.
 * @param full_name full message type name, i.e. the message type name
 * possibly with a package name prefix.
 * @return new instance of a protobuf message that has been registered
 * for the given message type.
 */
std::shared_ptr<google::protobuf::Message>
MessageRegister::new_message_for(std::string &full_name)
{
  if (message_by_typename_.find(full_name) == message_by_typename_.end()) {
    throw std::runtime_error("Message type not registered");
  }

  google::protobuf::Message *m = message_by_typename_[full_name]->New();
  return std::shared_ptr<google::protobuf::Message>(m);
}


/** Serialize a message.
 * @param component_id ID of component this message type belongs to
 * @param msg_type message type
 * @param msg message to seialize
 * @param frame_header upon return, the frame header is filled out according to
 * the given information and message.
 * @param data upon return, contains the serialized message
 */ 
void
MessageRegister::serialize(uint16_t component_id, uint16_t msg_type,
			   google::protobuf::Message &msg,
			   frame_header_t &frame_header, std::string &data)
{
  if (msg.SerializeToString(&data)) {
    frame_header.component_id = htons(component_id);
    frame_header.msg_type     = htons(msg_type);
    frame_header.payload_size = htonl(data.size());
  } else {
    throw std::runtime_error("Cannot serialize message");
  }
}


/** Deserialie message.
 * @param frame_header incoming message's frame header
 * @param data incoming message's data buffer
 * @return new instance of a protobuf message type that has been registered
 * for the given type.
 * @exception std::runtime_error thrown if anything goes wrong when
 * deserializing the message, e.g. if no protobuf message has been registered
 * for the given component ID and message type.
 */
std::shared_ptr<google::protobuf::Message>
MessageRegister::deserialize(frame_header_t &frame_header, void *data)
{
  uint16_t comp_id   = ntohs(frame_header.component_id);
  uint16_t msg_type  = ntohs(frame_header.msg_type);
  size_t   data_size = ntohl(frame_header.payload_size);

  std::shared_ptr<google::protobuf::Message> m =
    new_message_for(comp_id, msg_type);
  m->ParseFromArray(data, data_size);

  return m;
}

} // end namespace protobuf_comm
