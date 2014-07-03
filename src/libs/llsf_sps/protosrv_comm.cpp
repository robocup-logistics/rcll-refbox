
/***************************************************************************
 *  protosrv_comm.cpp - protobuf_comm server machine communication
 *
 *  Created: Thu Jul 03 11:07:20 2014
 *  Copyright  2013-2014  Tim Niemueller [www.niemueller.de]
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

#include <llsf_sps/protosrv_comm.h>

#include <core/exception.h>
#include <core/exceptions/software.h>
#include <utils/llsf/machines.h>

#include <msgs/BeaconSignal.pb.h>
#include <msgs/MachineCommands.pb.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>

using namespace protobuf_comm;

namespace llsf_sps {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define PSRV_NUM_MACHINES 16


/** @class ProtobufServerComm "protosrv_comm.h"
 * Communication for field machines connecting via protobuf_comm.
 * @author Tim Niemueller
 */


/** Constructor.
 * @param tcp_port TCP port to listen on for connections

 * @param wait_ms time in miliseconds to wait for connections, if set
 * to a non-zero value will throw an excetion if any machine has not connected
 * until the timeout
 */
ProtobufServerComm::ProtobufServerComm(unsigned short tcp_port,
				       unsigned int wait_ms)
  : MachineCommunication(PSRV_NUM_MACHINES)
{
  for (unsigned int i = 0; i < PSRV_NUM_MACHINES; ++i) {
    std::string mname    = to_string(i, llsf_utils::ASSIGNMENT_2014);
    MachineDetails &mdet = machines_[mname];
    mdet.puck_id   = NO_PUCK;
    mdet.connected = false;

    llsf_msgs::Machine *m = mdet.machine_info.add_machines();
    m->set_name(mname);
    llsf_msgs::LightSpec *r = m->add_lights();
    r->set_color(llsf_msgs::RED);
    r->set_state(llsf_msgs::ON);
    llsf_msgs::LightSpec *y = m->add_lights();
    y->set_color(llsf_msgs::YELLOW);
    y->set_state(llsf_msgs::ON);
    llsf_msgs::LightSpec *g = m->add_lights();
    g->set_color(llsf_msgs::GREEN);
    g->set_state(llsf_msgs::ON);
  }

  server_ = new protobuf_comm::ProtobufStreamServer(tcp_port);

  MessageRegister & message_register = server_->message_register();
  message_register.add_message_type<llsf_msgs::BeaconSignal>();
  message_register.add_message_type<llsf_msgs::RemovePuckFromMachine>();
  message_register.add_message_type<llsf_msgs::PlacePuckUnderMachine>();

  server_->signal_connected()
    .connect(boost::bind(&ProtobufServerComm::handle_client_connected, this, _1, _2));
  server_->signal_disconnected()
    .connect(boost::bind(&ProtobufServerComm::handle_client_disconnected, this, _1, _2));
  server_->signal_received()
    .connect(boost::bind(&ProtobufServerComm::handle_client_msg, this, _1, _2, _3, _4));
  server_->signal_receive_failed()
    .connect(boost::bind(&ProtobufServerComm::handle_client_fail, this, _1, _2, _3, _4));

  if (wait_ms > 0)  usleep(wait_ms * 1000);
  try_reconnect();
}


/** Destructor. */
ProtobufServerComm::~ProtobufServerComm()
{
  delete server_;
}

void
ProtobufServerComm::try_reconnect()
{
}


/** Reset all light register to zero (turn off all lights). */
void
ProtobufServerComm::reset_lights()
{
  for (auto m : machines_) {
    llsf_msgs::Machine   *mspec = m.second.machine_info.mutable_machines(0);
    for (int i = 0; i < 3; ++i) {
      llsf_msgs::LightSpec *lspec = mspec->mutable_lights(i);
      lspec->set_state(llsf_msgs::OFF);
    }
    if (m.second.connected) {
      server_->send(m.second.client_id, m.second.machine_info);
    }
  }
}

/** Set a light of a machine to given state.
 * @param m machine of which to set the light
 * @param light light color to set
 * @param state desired signal state of the light
 */
void
ProtobufServerComm::set_light(unsigned int m, Light light, SignalState state)
{
  std::string mname = to_string(m, llsf_utils::ASSIGNMENT_2014);

  if (machines_.find(mname) != machines_.end()) {    
    int lidx = -1;
    switch (light) {
    case LIGHT_YELLOW: lidx = 1; break;
    case LIGHT_GREEN:  lidx = 2; break;
    default:           lidx = 0; break;
    }

    llsf_msgs::Machine   *mspec = machines_[mname].machine_info.mutable_machines(0);
    llsf_msgs::LightSpec *lspec = mspec->mutable_lights(lidx);
    switch (state) {
    case SIGNAL_ON:     lspec->set_state(llsf_msgs::ON);    break;
    case SIGNAL_BLINK:  lspec->set_state(llsf_msgs::BLINK); break;
    default:            lspec->set_state(llsf_msgs::OFF);   break;
    }

    if (machines_[mname].connected) {
      server_->send(machines_[mname].client_id, machines_[mname].machine_info);
    }
  } else {
    return;
    //throw fawkes::Exception("Node %u unknown", m);
  }
}


/** Read puck ID via RFID.
 * @param m machine of which to read the puck
 * @param id upon returning true the read ID, unmodified otherwise
 * @return true if a puck was successfully read, false if there
 * is no puck under the reader or the communication was interrupted.
 */
bool
ProtobufServerComm::read_rfid(unsigned int m, uint32_t &id)
{
  if (! machine_exists(m)) {
    throw fawkes::Exception("Machine %u unknown", m);
  }

  std::string m_name = to_string(m, llsf_utils::ASSIGNMENT_2014);
  if (machines_.find(m_name) != machines_.end() && machines_[m_name].puck_id != NO_PUCK) {
    id = machines_[m_name].puck_id;
    return true;
  } else {
    return false;
  }
}

/** Read puck IDs via RFID.
 * @return vector of IDs read from all of the RFID machines. If an
 * idea is 0xFFFFFFFF then no puck was placed below the sensor.
 */
std::map<std::string, uint32_t>
ProtobufServerComm::read_rfids()
{
  std::map<std::string, uint32_t> rfid_tags;
  for (auto m : machines_) {
    rfid_tags[m.first] = m.second.puck_id;
  }
  return rfid_tags;
}


/** Reset all RFID registers.
 * Call this at the very beginning to avoid false values to be
 * written to a puck.
 */
void
ProtobufServerComm::reset_rfids()
{
  for (auto m : machines_) {
    m.second.puck_id = NO_PUCK;
  }
}


/** Write an ID to a puck using RFID.
 * @param m machine where to write
 * @param id ID to set on the puck
 */
void
ProtobufServerComm::write_rfid(unsigned int m, uint32_t id)
{
  throw fawkes::Exception("Not supported atm");
}


bool
ProtobufServerComm::machine_exists(unsigned int m)
{
  std::string mname = to_string(m, llsf_utils::ASSIGNMENT_2014);
  return ((machines_.find(mname) != machines_.end()) && machines_[mname].connected);
}


void
ProtobufServerComm::handle_client_connected(ProtobufStreamServer::ClientID client,
						   boost::asio::ip::tcp::endpoint &endpoint)
{
}


void
ProtobufServerComm::handle_client_disconnected(ProtobufStreamServer::ClientID client,
						      const boost::system::error_code &error)
{
  // remove machine detail about the disconnected machine
  for (auto m : machines_) {
    if (m.second.client_id == client) {
      m.second.connected = false;
    }
  }
}


/** Handle message that came from a client.
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message
 */
void
ProtobufServerComm::handle_client_msg(ProtobufStreamServer::ClientID client,
				      uint16_t component_id, uint16_t msg_type,
				      std::shared_ptr<google::protobuf::Message> msg)
{
  // beacon signal: update/add machine detail
  std::shared_ptr<llsf_msgs::BeaconSignal> bs;
  if ((bs = std::dynamic_pointer_cast<llsf_msgs::BeaconSignal>(msg))) {
    std::string mname = bs->peer_name();
    machine_ping(mname, client);
  }

  // place or remove message: update RFID tags
  std::shared_ptr<llsf_msgs::PlacePuckUnderMachine> pm;
  if ((pm = std::dynamic_pointer_cast<llsf_msgs::PlacePuckUnderMachine>(msg))) {
    std::string mname = pm->machine_name();
    if (machines_.find(mname) != machines_.end()) {
      machine_ping(mname, client);
      machines_[mname].puck_id = pm->puck_id();
    }
  }

  std::shared_ptr<llsf_msgs::RemovePuckFromMachine> rm;
  if ((rm = std::dynamic_pointer_cast<llsf_msgs::RemovePuckFromMachine>(msg))) {
    std::string mname = rm->machine_name();
    if (machines_.find(mname) != machines_.end()) {
      machine_ping(mname, client);
      machines_[mname].puck_id = NO_PUCK;
    }
  }

}

/** Handle server reception failure
 * @param client client ID
 * @param component_id component the message was addressed to
 * @param msg_type type of the message
 * @param msg the message string
 */
void
ProtobufServerComm::handle_client_fail(ProtobufStreamServer::ClientID client,
				       uint16_t component_id, uint16_t msg_type,
				       std::string msg)
{
  server_->disconnect(client);
  for (auto m : machines_) {
    if (m.second.client_id == client) {
      m.second.connected = false;
    }
  }
}


std::string
ProtobufServerComm::machine_name(protobuf_comm::ProtobufStreamServer::ClientID client_id)
{
  for (auto m : machines_) {
    if (m.second.client_id == client_id)  return m.first;
  }

  return "";
}


void
ProtobufServerComm::machine_ping(std::string &mname,
				 protobuf_comm::ProtobufStreamServer::ClientID client_id)
{
  if (machines_.find(mname) != machines_.end()) {
    if (! machines_[mname].connected) {
      // the machine was unknown before
      machines_[mname].connected = true;
      machines_[mname].client_id = client_id;

      // send current machine info
      server_->send(client_id, machines_[mname].machine_info);
    }
  }
}

} // end of namespace llsfrb
