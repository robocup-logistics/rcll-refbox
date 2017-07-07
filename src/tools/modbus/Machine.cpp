#include "Machine.h"
#include "MPSIoMapping.h"
#include "timeUtils.h"
#include "timeoutException.h"
#include <stdexcept>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

using namespace std;

Machine::Machine(): abort_operation_(false), in_registers_(4), out_registers_(4), connection_(nullptr) {
  for (int i = 0; i < 4; ++i) {
    in_registers_.push_back(0);
    out_registers_.push_back(0);
  }
}


void Machine::sendCommand(unsigned short command, unsigned short payload1, unsigned short payload2, int timeout, unsigned char status) {
  //cout << "Send Command " << command << endl;
  out_registers_[0] = command;
  out_registers_[1] = payload1;
  out_registers_[2] = payload2;
  out_registers_[3] = status;
  //std::lock_guard<std::mutex> g(lock_);
  if (not waitForReady(timeout))
    throw runtime_error("Previous command did not end within timeout");
  pushRegisters();
  if (status & STATUS_BUISY) {
    waitForBuisy();
  }
  if (timeout and (not waitForReady(timeout))) {
    ostringstream str;
    str << "Timeout during command #" << command << " (Timeout was " << timeout << ")";
    throw timeoutException(str.str());
  }
}

bool Machine::waitForReady(int timeout) {
  struct timespec time_c, time_0, time_d;
  if (timeout >= 0) {
    clock_gettime(CLOCK_MONOTONIC, &time_0);
  }
    
  do {
    {
      lock_guard<mutex> l(lock_);
      if (abort_operation_) {
        cout << "Abort operation" << endl;
        return false;
      }
      updateRegisters();
    }
    if (in_registers_.at(3) & STATUS_BUISY) {
      out_registers_[3] &= ~ STATUS_BUISY;
      pushRegisters();
    }
    if (timeout >= 0) {
      clock_gettime(CLOCK_MONOTONIC, &time_c);
      timespec_diff( &time_0, &time_c, &time_d);
      if( time_d.tv_sec * 1000 + time_d.tv_nsec / 1000000 > timeout) {
        return false;
      }
    }
      
  } while (! (in_registers_.at(3) & (STATUS_READY | STATUS_ERR)));
  if (in_registers_.at(3) & STATUS_READY)
    return true;
  else
    return false;
}

void Machine::waitForBuisy() {
  struct timespec time_c, time_0, time_d;
  clock_gettime(CLOCK_MONOTONIC, &time_0);
    
  do {
    updateRegisters();
    if (in_registers_.at(3) & STATUS_BUISY) {
      out_registers_[3] &= ~ STATUS_BUISY;
      pushRegisters();
      break;
    }
    clock_gettime(CLOCK_MONOTONIC, &time_c);
    timespec_diff( &time_0, &time_c, &time_d);
    if( time_d.tv_sec * 1000 + time_d.tv_nsec / 1000000 > TIMEOUT_BUISY) {
      throw timeoutException("Machine did not reset buisy flag within time limit");
    }
  } while (! (in_registers_.at(3) & (STATUS_READY | STATUS_ERR)));
}

bool Machine::waitForFree() {
  for(;;) {
    updateRegisters();
    if (not (in_registers_.at(3) & STATUS_BUISY)) {
      return true;
    }
  }
}


// TODO: do error handeling here.
//       eg throw error
//       and maybe add error to an error list for protobuf messages
void Machine::updateRegisters() {
  modbus_read_registers( connection_, 0, 4, (unsigned short*) in_registers_.data());
  // cout << "Read (" << in_registers_[0] << ", " << in_registers_[3] << ")" << endl;
}

// write out all registers to plc
void Machine::pushRegisters() {
  // cout << "Push (" << out_registers_[0] << ", " << out_registers_[3] << ")" << endl;
  modbus_write_registers( connection_, 0, 4, (unsigned short*) out_registers_.data());
}

void Machine::connectPLC(const std::string& ip, unsigned short port) {
  connection_ = modbus_new_tcp(ip.c_str(), port);
  if (modbus_connect(connection_)) {
    ostringstream o;
    o << "Connection to " << ip << " (" << port << ") failed:" << endl << modbus_strerror(errno);
    throw runtime_error(o.str());
  }
  updateRegisters();
  identify();
}

Machine::~Machine() {
  modbus_close(connection_);
  modbus_free(connection_);
}

  

void Machine::setLight(unsigned short color, llsf_msgs::LightState state, unsigned short time) {
  switch (color) {
    case LIGHT_RESET_CMD:
    case LIGHT_RED_CMD:
    case LIGHT_YELLOW_CMD:
    case LIGHT_GREEN_CMD:
      break;
    default: throw invalid_argument("Illegal color! See MPSIoMapping.h for choices.");
  }
  unsigned short int plc_state;
  switch (state) {
    case llsf_msgs::ON:
      plc_state = LIGHT_ON;
      break;
    case llsf_msgs::OFF:
      plc_state = LIGHT_OFF;
      break;
    case llsf_msgs::BLINK:
      plc_state = LIGHT_BLINK;
      break;
    default:
      plc_state = LIGHT_OFF;
      // TODO error
  }

  sendCommand( color, plc_state, time);
}


void Machine::resetLight() {
  setLight(LIGHT_RESET_CMD, llsf_msgs::OFF);
}
