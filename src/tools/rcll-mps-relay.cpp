
/***************************************************************************
 *  rcll-set-lights.cpp - set MPS lights
 *
 *  Created: Mon Mar 21 17:18:21 2016
 *  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
 *                  2017  Tobias Neumann
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

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include <config/yaml.h>

#include <protobuf_comm/server.h>
#include <utils/system/argparser.h>

#include <msgs/MachineInstructions.pb.h>
#include <msgs/MachineDescription.pb.h>

#include <thread>         // std::thread
#include <mutex>          // std::mutex
#include <iostream>

#include "modbus/MPSIoMapping.h"
#include "modbus/BaseStation.h"
#include "modbus/StorageStation.h"
#include "modbus/DeliveryStation.h"
#include "modbus/RingStation.h"
#include "modbus/CapStation.h"

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

boost::asio::io_service io_service_;

static bool quit = false;
ProtobufStreamServer *server_ = NULL;

std::string machine_name_;
int id_last_light_ = -1;
int id_last_mps_interupt_ = -1;
int id_last_process_ = -1;
std::map<int, std::shared_ptr<llsf_msgs::MachineReply>> finished_ids_;

std::mutex request_pull_mutex_;
bool request_pull_ = false;
std::shared_ptr<std::thread> request_pull_thread_;

std::mutex mps_msg_;
unsigned int bases_added_since_last_poll_ = 0;
std::list<int> barcodes_read_since_last_poll_;
std::shared_ptr<std::thread> mps_msg_thread_;

static std::shared_ptr<llsfrb::modbus::Machine> mps_mb_;

static std::string plc_ip_ = "192.168.2.27";
static unsigned short int plc_port_ = 502;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
    io_service_.stop();
  }
}

void
handle_connected(ProtobufStreamServer::ClientID client,
                 boost::asio::ip::tcp::endpoint &endpoint)
{
  printf("Connected to RefBox\n");
}

void
handle_disconnected(ProtobufStreamServer::ClientID client,
                    const boost::system::error_code &error)
{
  printf("Disconected from RefBox\n");
//  quit = true;
//  io_service_.stop();
}

void
handle_receive_failed(ProtobufStreamServer::ClientID client,
                      uint16_t component_id, uint16_t msg_type,
                      std::string msg)
{
  printf("Received failed to %u: comp_id: %u, msg_type: %u\n", client, component_id, msg_type);
}

void
mps_msg_thread()
{
  std::cout << std::endl
            << std::endl
            << "You can input '+'s to add bases or write numbers to create a barcode." << std::endl
            << "The line will be interpred on pressing enter." << std::endl
            << std::endl;
  do {
    std::string input_line = "";
    std::cin >> input_line;
    std::string input_barcode = "";
    unsigned int input_bases = 0;
    for (char& c : input_line ) {
      if (c == '+') {
        input_bases++;
      }
      if (c >= '0' && c <= '9') {
        input_barcode += c;
      }
    }
    std::cout << std::endl
              << "Base added: " << input_bases << std::endl
              << "Barcode: " << input_barcode << std::endl;

    mps_msg_.lock();
    bases_added_since_last_poll_ += input_bases;
    if ( ! input_barcode.empty() ) {
      barcodes_read_since_last_poll_.push_back( std::stoi( input_barcode) );
    }
    mps_msg_.unlock();

    request_pull_mutex_.lock();
    request_pull_ = true;
    request_pull_mutex_.unlock();

    sleep(1);
  } while ( ! quit );
}

void
request_pull_thread()
{
  do {
    // copy shared variable to local
    request_pull_mutex_.lock();
    bool request_pull_local = request_pull_;
    request_pull_mutex_.unlock();
    // when I have local data, request refbox for pull
    if ( request_pull_local ) {
      printf("send pull request\n");
      llsf_msgs::MachineRequestPull pull_request;
      pull_request.set_machine(machine_name_);
      server_->send_to_all( pull_request );
    }
    sleep(1);
  } while( ! quit );
}

bool
is_new_msg_id(std::shared_ptr<llsf_msgs::InstructMachine> msg)
{
  int id = msg->id();
  switch ( msg->set() ) {
    case llsf_msgs::INSTRUCT_MACHINE_SET_SIGNAL_LIGHT:
      if ( id > id_last_light_ ) {
        id_last_light_ = id;
        return true;
      } else {
        return false;
      }
      break;
    case llsf_msgs::INSTRUCT_MACHINE_PULL_MSGS_FROM_MPS:
      if ( id > id_last_mps_interupt_ ) {
        id_last_mps_interupt_ = id;
        return true;
      } else {
        return false;
      }
      break;
    default:
      if ( id > id_last_process_ ) {
        id_last_process_ = id;
        return true;
      } else {
        return false;
      }
  }
}

void
reset_machine()
{
  id_last_light_        = -1;
  id_last_mps_interupt_ = -1;
  id_last_process_      = -1;
  bases_added_since_last_poll_ = 0;
  barcodes_read_since_last_poll_.clear();
  finished_ids_.clear();
}

void
send_reply(std::shared_ptr<llsf_msgs::MachineReply> msg)
{
  server_->send_to_all( *msg );
}

void
send_reply_and_append(int id)
{
  std::shared_ptr<llsf_msgs::MachineReply> reply(new llsf_msgs::MachineReply);
  reply->set_id( id );
  reply->set_machine( machine_name_ );
  reply->set_set( llsf_msgs::MACHINE_REPLY_FINISHED );

  send_reply( reply );

  finished_ids_[id] = reply;
}

void
send_reply_and_append(int id, std::list<std::shared_ptr<llsf_msgs::MachineSensorInfo>> sensors)
{
  std::shared_ptr<llsf_msgs::MachineReply> reply(new llsf_msgs::MachineReply);
  reply->set_id( id );
  reply->set_machine( machine_name_ );
  reply->set_set( llsf_msgs::MACHINE_REPLY_FINISHED_WITH_PAYLOAD );

  for (std::shared_ptr<llsf_msgs::MachineSensorInfo> sensor : sensors) {
    llsf_msgs::MachineSensorInfo* new_sensor = reply->add_sensors();
    *new_sensor = *sensor;
  }

  send_reply( reply );

  finished_ids_[id] = reply;
}


void
check_id_for_finised_msgs_and_send_reply(int id)
{
  std::map<int, std::shared_ptr<llsf_msgs::MachineReply>>::const_iterator id_pos = finished_ids_.find(id);
  if ( id_pos != finished_ids_.end() ) {  // id found
    send_reply( id_pos->second );
  }
  // else do nothing, old or not yet finished msg
}

void
handle_message(ProtobufStreamServer::ClientID client,
               uint16_t component_id, uint16_t msg_type,
               std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<llsf_msgs::InstructMachine> im;
  if ( (im = std::dynamic_pointer_cast<llsf_msgs::InstructMachine>(msg)) ) {
    std::string machine = im->machine();
    if (machine == machine_name_) { // if this machine is running here
      int id = im->id();
      // special case for reset
      if (llsf_msgs::INSTRUCT_MACHINE_RESET == im->set()) {
        printf("Received reset for machine\n");

        // since this is always generated and send when a reset is received (even with any ID), this also covers the case of a lost reply msg
        send_reply_and_append(id);
        reset_machine();  // need to be reset afterwards, to also clear the just appended msg

        return; // we need to stop here!!!, otherwise the ids gets back up again
      }

      if ( ! is_new_msg_id( im ) ) {
        check_id_for_finised_msgs_and_send_reply( id );
      } else {
        // generic stuff for machines
        switch ( im->set() ) {
          case llsf_msgs::INSTRUCT_MACHINE_SET_SIGNAL_LIGHT:
            printf("Set light to: (ryg): %s, %s, %s\n",
                   llsf_msgs::LightState_Name( im->light_state().red() ).c_str(),
                   llsf_msgs::LightState_Name( im->light_state().yellow() ).c_str(),
                   llsf_msgs::LightState_Name( im->light_state().green() ).c_str()
                  );
            mps_mb_->set_light(llsf_msgs::LightColor::RED, im->light_state().red());
            mps_mb_->set_light(llsf_msgs::LightColor::YELLOW, im->light_state().yellow());
            mps_mb_->set_light(llsf_msgs::LightColor::GREEN, im->light_state().green());
            send_reply_and_append(id);
            break;
          case llsf_msgs::INSTRUCT_MACHINE_MOVE_CONVEYOR:
            printf("Move conveyor to %s and stop at %s\n",
                   llsf_msgs::ConveyorDirection_Name( im->conveyor_belt().direction() ).c_str(),
                   llsf_msgs::SensorOnMPS_Name( im->conveyor_belt().stop_sensor() ).c_str()
                  );
            send_reply_and_append(id);
            break;
          case llsf_msgs::INSTRUCT_MACHINE_STOP_CONVEYOR:
            printf("STOP! conveyor\n");
            send_reply_and_append(id);
            break;
          case llsf_msgs::INSTRUCT_MACHINE_WAIT_FOR_PICKUP:
            printf("Wait for pickup at TODO-fix-msg-and-send-sensor grace-time is TODO-add change light to (ryg) %s %s %s in case the product is picked up\n",
                   llsf_msgs::LightState_Name( im->light_state().red() ).c_str(),
                   llsf_msgs::LightState_Name( im->light_state().yellow() ).c_str(),
                   llsf_msgs::LightState_Name( im->light_state().green() ).c_str()
                  );
            send_reply_and_append(id);
            break;
          case llsf_msgs::INSTRUCT_MACHINE_BS:
            if ( im->machine().find("BS") != std::string::npos ) {  // if it is a BS
              printf("Pushout from feeder # %u\n", im->bs().slot() );
              send_reply_and_append(id);
            } else {
              printf("Error, unknown \"set\": %u\n", im->set());
            }
            break;
          case llsf_msgs::INSTRUCT_MACHINE_SS:
            if ( im->machine().find("SS") != std::string::npos ) {
              printf("%s from slot %u %u %u\n",
                     llsf_msgs::SSOp_Name( im->ss().operation() ).c_str(),
                     im->ss().slot().x(),
                     im->ss().slot().y(),
                     im->ss().slot().z()
                    );
              send_reply_and_append(id);
            } else {
              printf("Error, unknown \"set\": %u\n", im->set());
            }
            break;
          case llsf_msgs::INSTRUCT_MACHINE_DS:
            if ( im->machine().find("DS") != std::string::npos ) {
              printf("Set to gate %u\n", im->ds().gate() );
              send_reply_and_append(id);
            } else {
              printf("Error, unknown \"set\": %u\n", im->set());
            }
            break;
          case llsf_msgs::INSTRUCT_MACHINE_CS:
            if ( im->machine().find("CS") != std::string::npos ) {
              printf("%s cap\n", llsf_msgs::CSOp_Name( im->cs().operation() ).c_str() );
              send_reply_and_append(id);
            } else {
              printf("Error, unknown \"set\": %u\n", im->set());
            }
            break;
          case llsf_msgs::INSTRUCT_MACHINE_RS:
            if ( im->machine().find("RS") != std::string::npos ) {
              printf("Mount ring from feeder %u\n", im->rs().feeder() );
              send_reply_and_append(id);
            } else {
              printf("Error, unknown \"set\": %u\n", im->set());
            }
            break;
          case llsf_msgs::INSTRUCT_MACHINE_PULL_MSGS_FROM_MPS:
            {
              // stop sending the pull request
              request_pull_mutex_.lock();
              request_pull_ = false;
              request_pull_mutex_.unlock();

              // transfer all data that I gatherd from my sensors into the reply msg
              std::list<std::shared_ptr<llsf_msgs::MachineSensorInfo>> sensors;
              mps_msg_.lock();
              if (bases_added_since_last_poll_ != 0) {
                std::shared_ptr<llsf_msgs::MachineSensorInfo> s_base(new llsf_msgs::MachineSensorInfo);
                s_base->set_added_bases( bases_added_since_last_poll_ );
                sensors.push_back(s_base);
              }
              for (int barcode : barcodes_read_since_last_poll_) {
                std::shared_ptr<llsf_msgs::MachineSensorInfo> s_barcode(new llsf_msgs::MachineSensorInfo);
                s_barcode->set_barcode(barcode);
                sensors.push_back(s_barcode);
              }

              bases_added_since_last_poll_ = 0;
              barcodes_read_since_last_poll_.clear();
              mps_msg_.unlock();

              send_reply_and_append(id, sensors);
            }
            break;
          default:
            // this should never be reached
            printf("ERROR in code!!!, this should not be reached");
        }
      }
    }
  }
}



void
usage(const char *progname)
{
  printf("Usage: %s [-R host[:port]] -m <machine-name>\n",
	 progname);
}


int
main(int argc, char **argv)
{
  reset_machine();
  ArgumentParser argp(argc, argv, "m:R");

  if ( ! (argp.has_arg("m")) ) {
    usage(argv[0]);
    exit(1);
  }
  machine_name_ = argp.arg("m");

  if (machine_name_ == "C-BS" || machine_name_ == "M-BS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::BaseStation>(new llsfrb::modbus::BaseStation()));
  } else if (machine_name_ == "C-SS" || machine_name_ == "M-SS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::StorageStation>(new llsfrb::modbus::StorageStation()));
  } else if (machine_name_ == "C-DS" || machine_name_ == "M-DS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::DeliveryStation>(new llsfrb::modbus::DeliveryStation()));
  } else if (machine_name_ == "C-RS" || machine_name_ == "M-RS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::RingStation>(new llsfrb::modbus::RingStation()));
  } else if (machine_name_ == "C-CS" || machine_name_ == "M-CS" ) {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::CapStation>(new llsfrb::modbus::CapStation()));
  } else {
    std::cout << "Machine of type " << machine_name_ << " is not yet implemented" << std::endl
              << "stop programm" << std::endl;
    return 0;
  }


  llsfrb::YamlConfiguration *config_ = new llsfrb::YamlConfiguration(CONFDIR);
  config_->load("config.yaml");
  std::string cfg_prefix = std::string("/llsfrb/mps/stations/");
  std::string host = "127.0.0.1"; // config_->get_string( (cfg_prefix + machine_name_ + "/host").c_str() );
  unsigned int port = 502; //config_->get_uint( (cfg_prefix + machine_name_ + "/port").c_str() );

  char* ip;
  unsigned short int p;
  if ( argp.parse_hostport("R", &ip, &p) ) {
    plc_ip_ = ip;
    plc_port_ = p;
  }

  mps_mb_->connect_PLC(plc_ip_, plc_port_);

  MessageRegister * message_register = new MessageRegister();
  message_register->add_message_type<llsf_msgs::InstructMachine>();
  server_ = new ProtobufStreamServer(port, message_register);

  server_->signal_connected().connect(handle_connected);
  server_->signal_disconnected().connect(handle_disconnected);
  server_->signal_received().connect(handle_message);
  server_->signal_receive_failed().connect(handle_receive_failed);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  mps_msg_thread_ = std::shared_ptr<std::thread>(new std::thread(mps_msg_thread));
  request_pull_thread_ = std::shared_ptr<std::thread>(new std::thread(request_pull_thread));

  do {
    io_service_.run();
    io_service_.reset();
  } while (! quit);

//  mps_msg_thread_->join();
  request_pull_thread_->join();

  delete server_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}

