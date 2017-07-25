#include "Machine.h"
#include "MPSIoMapping.h"
#include "timeUtils.h"
#include "timeoutException.h"
#include <stdexcept>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

Machine::Machine(unsigned short int machine_type) : abort_operation_(false), machine_type_(machine_type), in_registers_(4), out_registers_(4), connection_(nullptr) {
  for (int i = 0; i < 4; ++i) {
    in_registers_.push_back(0);
    out_registers_.push_back(0);
  }
}


void Machine::send_command(unsigned short command, unsigned short payload1, unsigned short payload2, int timeout, unsigned char status) {
  //cout << "Send Command " << command << endl;
  out_registers_[0] = command;
  out_registers_[1] = payload1;
  out_registers_[2] = payload2;
  out_registers_[3] = status;
  //std::lock_guard<std::mutex> g(lock_);
//  if (not wait_for_ready(timeout))
//    throw std::runtime_error("Previous command did not end within timeout");
  push_registers();
  if (status & Status::STATUS_BUISY) {
    wait_for_buisy();
  }
  if (timeout and (not wait_for_ready(timeout))) {
    std::ostringstream str;
    str << "Timeout during command #" << command << " (Timeout was " << timeout << ")";
    throw timeout_exception(str.str());
  }
}

void Machine::reset() {
  send_command(machine_type_ | Command::COMMAND_RESET);
}

bool Machine::wait_for_ready(int timeout) {
  struct timespec time_c, time_0, time_d;
  if (timeout >= 0) {
    clock_gettime(CLOCK_MONOTONIC, &time_0);
  }
    
  do {
    {
      std::lock_guard<std::mutex> l(lock_);
      if (abort_operation_) {
        std::cout << "Abort operation" << std::endl;
        return false;
      }
      update_registers();
    }
    if (in_registers_.at(3) & Status::STATUS_BUISY) {
      out_registers_[3] &= ~ Status::STATUS_BUISY;
      push_registers();
    }
    if (timeout >= 0) {
      clock_gettime(CLOCK_MONOTONIC, &time_c);
      timespec_diff( &time_0, &time_c, &time_d);
      if( time_d.tv_sec * 1000 + time_d.tv_nsec / 1000000 > timeout) {
        return false;
      }
    }
      
  } while (! (in_registers_.at(3) & (Status::STATUS_READY | Status::STATUS_ERR)));
  if (in_registers_.at(3) & Status::STATUS_READY)
    return true;
  else
    return false;
}

void Machine::wait_for_buisy() {
  struct timespec time_c, time_0, time_d;
  clock_gettime(CLOCK_MONOTONIC, &time_0);
    
  do {
    update_registers();
    if (in_registers_.at(3) & Status::STATUS_BUISY) {
      out_registers_[3] &= ~ Status::STATUS_BUISY;
      push_registers();
      break;
    }
    clock_gettime(CLOCK_MONOTONIC, &time_c);
    timespec_diff( &time_0, &time_c, &time_d);
    if( time_d.tv_sec * 1000 + time_d.tv_nsec / 1000000 > Timeout::TIMEOUT_BUISY) {
      throw timeout_exception("Machine did not reset buisy flag within time limit");
    }
  } while (! (in_registers_.at(3) & (Status::STATUS_READY | Status::STATUS_ERR)));
}

bool Machine::wait_for_free() {
  for(;;) {
    update_registers();
    if (not (in_registers_.at(3) & Status::STATUS_BUISY)) {
      return true;
    }
  }
}


// TODO: do error handeling here.
//       eg throw error
//       and maybe add error to an error list for protobuf messages
void Machine::update_registers() {
  modbus_read_registers( connection_, 0, 4, (unsigned short*) in_registers_.data());
  // cout << "Read (" << in_registers_[0] << ", " << in_registers_[3] << ")" << endl;
}

// write out all registers to plc
void Machine::push_registers() {
  // cout << "Push (" << out_registers_[0] << ", " << out_registers_[3] << ")" << endl;
  modbus_write_registers( connection_, 0, 4, (unsigned short*) out_registers_.data());
}

void Machine::connect_PLC(const std::string& ip, unsigned short port) {
  connection_ = modbus_new_tcp(ip.c_str(), port);
  if (modbus_connect(connection_)) {
    std::ostringstream o;
    o << "Connection to " << ip << " (" << port << ") failed:" << std::endl << modbus_strerror(errno);
    throw std::runtime_error(o.str());
  }
  update_registers();
  identify();
}

Machine::~Machine() {
  modbus_close(connection_);
  modbus_free(connection_);
}

  

void Machine::set_light(llsf_msgs::LightColor color, llsf_msgs::LightState state, unsigned short time) {
  LightColor m_color;
  switch (color) {
    case llsf_msgs::LightColor::RED:
      m_color = LightColor::LIGHT_COLOR_RED;
      break;
    case llsf_msgs::LightColor::YELLOW:
      m_color = LightColor::LIGHT_COLOR_YELLOW;
      break;
    case llsf_msgs::LightColor::GREEN:
      m_color = LightColor::LIGHT_COLOR_GREEN;
      break;
  }
  switch (m_color) {
    case LightColor::LIGHT_COLOR_RESET:
    case LightColor::LIGHT_COLOR_RED:
    case LightColor::LIGHT_COLOR_YELLOW:
    case LightColor::LIGHT_COLOR_GREEN:
      break;
    default: throw std::invalid_argument("Illegal color! See MPSIoMapping.h for choices.");
  }
  unsigned short int plc_state;
  switch (state) {
    case llsf_msgs::ON:
      plc_state = LightState::LIGHT_STATE_ON;
      break;
    case llsf_msgs::OFF:
      plc_state = LightState::LIGHT_STATE_OFF;
      break;
    case llsf_msgs::BLINK:
      plc_state = LightState::LIGHT_STATE_BLINK;
      break;
    default:
      plc_state = LightState::LIGHT_STATE_OFF;
      // TODO error
  }
  std::cout << "Send: " << m_color << " " << plc_state << " "  << time << std::endl;
  send_command( m_color, plc_state, time);
}

void Machine::conveyor_move(llsf_msgs::ConveyorDirection direction, llsf_msgs::SensorOnMPS sensor)
{
  unsigned short sensor_sps = -1;
  switch (sensor) {
    case llsf_msgs::SensorOnMPS::SENSOR_INPUT:
      sensor_sps = 1;
      break;
    case llsf_msgs::SensorOnMPS::SENSOR_MIDDLE:
      sensor_sps = 2;
      break;
    case llsf_msgs::SensorOnMPS::SENSOR_OUTPUT:
      sensor_sps = 3;
      break;
    default:
      std::cout << "Wrong sensor send" << std::endl;
      break;
  }
  send_command(Command::COMMAND_MOVE_CONVEYOR + machine_type_, sensor_sps, direction, Timeout::TIMEOUT_BAND);
}

void Machine::reset_light() {
  set_light(llsf_msgs::LightColor::RED, llsf_msgs::OFF);
}

}
}
