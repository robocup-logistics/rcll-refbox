#include "mps.h"
#include <cstring>
#include <iostream>

MPS::MPS(const char* ip, int port) {
  this->ip = strdup(ip);
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  machineState = IDLE;
  
  if(modbus_connect(this->mb) == -1) {
    machineState = DISCONNECTED;
  }

  setType();
}

MPS::~MPS() {
  free(ip);
  machineState = DISCONNECTED;
  modbus_close(this->mb);
  modbus_free(this->mb);
}

void MPS::reconnect() {
  modbus_close(mb);
  if (modbus_connect(mb) == -1) {
    machineState = DISCONNECTED;
  }
}

std::string MPS::machienStateString() {
  switch(machineState) {
    case IDLE:
      return "IDLE";
      break;
    case AVAILABLE:
      return "AVAILABLE";
      break;
    case PROCESSING:
      return "PROCESSING";
      break;
    case PROCESSED:
      return "PROCESSED";
      break;
    case DELIVER:
      return "DELIVER";
      break;
    case DELIVERED:
      return "DELIVERED";
      break;
    case RETRIEVED:
      return "RETRIEVED";
      break;
    case DISCONNECTED:
      return "DISCONNECTED";
      break;
    default:
      return "ERROR";
      break;
  }
}

void MPS::setType() {
  uint16_t send[1] = {(uint16_t) type};
  int rc = modbus_write_registers(mb, 11, 1, send);
  
  if(rc == -1) {
    machineState = DISCONNECTED;
  }
}
