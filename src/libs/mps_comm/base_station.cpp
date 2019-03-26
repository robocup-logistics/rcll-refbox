#include "base_station.h"
#include "mps_io_mapping.h"
#include <iostream>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

BaseStation::BaseStation() : Machine(Station::STATION_BASE) { }
BaseStation::~BaseStation() {}

void BaseStation::band_on_until_in() {
  send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_IN);
}

void BaseStation::band_on_until_mid() {
  send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_MID);
}

void BaseStation::band_on_until_out() {
  send_command(Operation::OPERATION_BAND_ON_UNTIL + machine_type_, Operation::OPERATION_BAND_OUT);
}

void BaseStation::get_base(llsf_msgs::BaseColor color) {
  //lock_guard<mutex> g(lock_);
  llsfrb::modbus::BaseColor color_sps;;
  switch (color) {
    case llsf_msgs::BASE_RED:
      color_sps = llsfrb::modbus::BaseColor::BASE_COLOR_RED;
      break;
    case llsf_msgs::BASE_BLACK:
      color_sps = llsfrb::modbus::BaseColor::BASE_COLOR_BLACK;
      break;
    case llsf_msgs::BASE_SILVER:
      color_sps = llsfrb::modbus::BaseColor::BASE_COLOR_SILVER;
      break;
    default:
      std::cout << "Error in RefBox, NOT implemented base color" << std::endl;
      return;
  }

  send_command(machine_type_ + Operation::OPERATION_GET_BASE, color_sps);
  wait_for_ready();
}

// Need information on how to access this
bool BaseStation::base_ready() {
  std::cout << "Not implemented yet!" << std::endl;
  return true;
}

// Need information on how to access this
bool BaseStation::is_empty() {
  std::cout << "Not implemented yet!" << std::endl;
  return false;
}

void BaseStation::identify() {
  send_command(Command::COMMAND_SET_TYPE, StationType::STATION_TYPE_BS);
}

}
}
