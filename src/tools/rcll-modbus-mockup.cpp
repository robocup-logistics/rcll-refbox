
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

static std::shared_ptr<Machine> modbus_if_;
static std::string plc_ip_ = "127.0.0.1";
static unsigned short int plc_port_ = 5000;

void
usage(const char *progname)
{
  printf("Usage: %s <machine-name> <instructions> [-R host[:port]]\n"
         "\n"
         "general instructions\n"
         "light: (in the order red, yellow, green) (OFF|ON|BLINK) (OFF|ON|BLINK) (OFF|ON|BLINK)\n"
         "conveyor: (FORWARD|BACKWARD) (SENSOR_INPUT|SENSOR_OUTPUT|SENSOR_MIDDLE)\n"
         "reset\n"
/*         "instructions are specific for the machine type:\n"
         "BS:  (INPUT|OUTPUT) (BASE_RED|BASE_BLACK|BASE_SILVER)\n"
         "DS:  <gate number>\n"
         "SS:  (RETRIEVE|STORE) <slot-x> <slot-y> <slot-z>\n"
         "RS:  (RING_BLUE|RING_GREEN|RING_ORANGE|RING_YELLOW)\n"
         "CS:  (RETRIEVE_CAP|MOUNT_CAP)\n"*/,
         progname);
}


int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "T:R");

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
    modbus_if_ = std::dynamic_pointer_cast<Machine>(std::shared_ptr<BaseStation>(new BaseStation()));
  } else if (machine_type_ == "SS") {
    modbus_if_ = std::dynamic_pointer_cast<Machine>(std::shared_ptr<StorageStation>(new StorageStation()));
  } else if (machine_type_ == "DS") {
    modbus_if_ = std::dynamic_pointer_cast<Machine>(std::shared_ptr<DeliveryStation>(new DeliveryStation()));
  } else if (machine_type_ == "RS") {
    modbus_if_ = std::dynamic_pointer_cast<Machine>(std::shared_ptr<RingStation>(new RingStation()));
  } else if (machine_type_ == "CS") {
    modbus_if_ = std::dynamic_pointer_cast<Machine>(std::shared_ptr<CapStation>(new CapStation()));
  } else {
    std::cout << "Machine of type " << machine_type_ << " is not yet implemented" << std::endl
              << "stop programm" << std::endl;
    return 0;
  }

  modbus_if_->connectPLC(plc_ip_, plc_port_);

  if (machine_command_ == "light") {
    llsf_msgs::LightState r, y, g;
    if (! llsf_msgs::LightState_Parse(argp.items()[2], &r)) {
      printf("Invalid color for red: %s\n", argp.items()[2]);
      exit(-2);
    }
    if (! llsf_msgs::LightState_Parse(argp.items()[3], &y)) {
      printf("Invalid color for yellow: %s\n", argp.items()[3]);
      exit(-2);
    }
    if (! llsf_msgs::LightState_Parse(argp.items()[4], &g)) {
      printf("Invalid color for green: %s\n", argp.items()[4]);
      exit(-2);
    }
    std::cout << "Set light color to: " << llsf_msgs::LightState_Name(r) << " " << llsf_msgs::LightState_Name(y) << " " << llsf_msgs::LightState_Name(g) << std::endl;

    modbus_if_->setLight(LIGHT_RED_CMD, r);
    modbus_if_->setLight(LIGHT_YELLOW_CMD, y);
    modbus_if_->setLight(LIGHT_GREEN_CMD, g);
  } else if (machine_command_ == "conveyor") {
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

    modbus_if_->conveyor_move(direction, sensor);
  } else if (machine_command_ == "reset") {
    std::cout << "resetting " << machine_name_ << std::endl;

    modbus_if_->reset();
  } else {
    std::cout << "Command unknown or not possible for the given machine " << std::endl
              << "stop programm" << std::endl;
    return 0;
  }

  return 0;
}
