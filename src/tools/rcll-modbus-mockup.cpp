
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
#include <msgs/MachineInstructions.pb.h>

#include <utils/system/argparser.h>

#include <iostream>

#include "modbus/MPSIoMapping.h"
#include "modbus/BaseStation.h"
#include "modbus/StorageStation.h"
#include "modbus/DeliveryStation.h"
#include "modbus/RingStation.h"
#include "modbus/CapStation.h"

using namespace fawkes;

static std::string machine_name_;
static std::string machine_type_;
static std::string machine_command_;
//static int id_last_ = 0;

static std::shared_ptr<llsfrb::modbus::Machine> mps_mb_;
//static std::string plc_ip_ = "192.168.2.26";
//static unsigned short int plc_port_ = 502;
static std::string plc_ip_ = "127.0.0.1";
static unsigned short int plc_port_ = 5000;

void
usage(const char *progname)
{
  printf("Usage: %s <machine-name> <instructions> [-R host[:port]]\n"
         "\n"
         "general instructions\n"
         "light: (RED|YELLOW|GREEN) (OFF|ON|BLINK)\n"
         "conveyor: (FORWARD|BACKWARD) (SENSOR_INPUT|SENSOR_OUTPUT|SENSOR_MIDDLE)\n"
         "reset\n"
         "instructions are specific for the machine type:\n"
         "BS: (BASE_RED|BASE_BLACK|BASE_SILVER)\n"
/*         "DS:  <gate number>\n"
         "SS:  (RETRIEVE|STORE) <slot-x> <slot-y> <slot-z>\n"
*/
         "RS: (number of feeder) (0|1)\n"
/*         "CS:  (RETRIEVE_CAP|MOUNT_CAP)\n"*/,
         progname);
}


int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "R:");

  if (argp.num_items() < 2) {
    usage(argv[0]);
    exit(1);
  }

  machine_name_    = argp.items()[0];
  machine_type_    = machine_name_.substr(2, 2);
  machine_command_ = argp.items()[1];

  char* host;
  unsigned short int port;
  if ( argp.parse_hostport("R", &host, &port) ) {
    plc_ip_ = host;
    plc_port_ = port;
  }

//  llsfrb::YamlConfiguration *config_ = new llsfrb::YamlConfiguration(CONFDIR);
//  config_->load("config.yaml");
//  std::string cfg_prefix = std::string("/llsfrb/mps/stations/");
//  std::string host = config_->get_string( (cfg_prefix + machine_name_ + "/host").c_str() );
//  unsigned int port = config_->get_uint( (cfg_prefix + machine_name_ + "/port").c_str() );

  if (machine_type_ == "BS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::BaseStation>(new llsfrb::modbus::BaseStation()));
  } else if (machine_type_ == "SS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::StorageStation>(new llsfrb::modbus::StorageStation()));
  } else if (machine_type_ == "DS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::DeliveryStation>(new llsfrb::modbus::DeliveryStation()));
  } else if (machine_type_ == "RS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::RingStation>(new llsfrb::modbus::RingStation()));
  } else if (machine_type_ == "CS") {
    mps_mb_ = std::dynamic_pointer_cast<llsfrb::modbus::Machine>(std::shared_ptr<llsfrb::modbus::CapStation>(new llsfrb::modbus::CapStation()));
  } else {
    std::cout << "Machine of type " << machine_type_ << " is not yet implemented" << std::endl
              << "stop programm" << std::endl;
    return 0;
  }

  mps_mb_->connect_PLC(plc_ip_, plc_port_);

  if (machine_command_ == "light") {
    if (argp.num_items() <= 3) {
      usage(argv[0]);
      exit(1);
    }
    llsf_msgs::LightColor color;
    llsf_msgs::LightState state;
    if (! llsf_msgs::LightColor_Parse(argp.items()[2], &color)) {
      printf("Invalid color, is: %s\n", argp.items()[2]);
      exit(-2);
    }
    if (! llsf_msgs::LightState_Parse(argp.items()[3], &state)) {
      printf("Invalid state, is: %s\n", argp.items()[3]);
      exit(-2);
    }
    std::cout << "Set light color " << llsf_msgs::LightColor_Name(color) << " to: " << llsf_msgs::LightState_Name(state) << std::endl;

    mps_mb_->set_light(color, state);
  } else if (machine_command_ == "conveyor") {
    if (argp.num_items() <= 3) {
      usage(argv[0]);
      exit(1);
    }
    llsf_msgs::ConveyorDirection direction;
    llsf_msgs::SensorOnMPS sensor;
    if (! llsf_msgs::ConveyorDirection_Parse(argp.items()[2], &direction)) {
      printf("Invalid direction: %s\n", argp.items()[2]);
      exit(-2);
    }
    if (! llsf_msgs::SensorOnMPS_Parse(argp.items()[3], &sensor)) {
      printf("Invalid sensor: %s\n", argp.items()[3]);
      exit(-2);
    }
    std::cout << "move conveyor " << llsf_msgs::ConveyorDirection_Name(direction) << " till " << llsf_msgs::SensorOnMPS_Name(sensor)  << std::endl;

    mps_mb_->conveyor_move(direction, sensor);
  } else if (machine_command_ == "reset") {
    std::cout << "resetting " << machine_name_ << std::endl;

    mps_mb_->reset();
  } else if (machine_command_ == "BS") {
    std::shared_ptr<llsfrb::modbus::BaseStation> bs = std::dynamic_pointer_cast<llsfrb::modbus::BaseStation>(mps_mb_);
    if (argp.num_items() <= 2) {
      usage(argv[0]);
      exit(1);
    }
    llsf_msgs::BaseColor base_color;
    if (! llsf_msgs::BaseColor_Parse(argp.items()[2], &base_color)) {
      printf("Invalid dispencer: %s\n", argp.items()[2]);
      exit(-2);
    }
    std::cout << "BS push out base " << llsf_msgs::BaseColor_Name(base_color) << std::endl;

    bs->get_base(base_color);
  } else if (machine_command_ == "RS") {
    std::shared_ptr<llsfrb::modbus::RingStation> rs = std::dynamic_pointer_cast<llsfrb::modbus::RingStation>(mps_mb_);
    if (argp.num_items() <= 2) {
      usage(argv[0]);
      exit(1);
    }
    unsigned int feeder = std::stoi( argp.items()[2] );
    std::cout << "RS mount from Feeder # " << feeder << std::endl;

    rs->mount_ring( feeder );
  } else {
    std::cout << "Command unknown or not possible for the given machine " << std::endl
              << "stop programm" << std::endl;
    return 0;
  }

  return 0;
}
