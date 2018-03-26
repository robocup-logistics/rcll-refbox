#include "mps.h"
#include <cstring>
#include <iostream>

MPS::MPS(const char* ip, int port) {
  this->ip = strdup(ip);
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  // set new timeout
  struct timeval response_timeout;
  response_timeout.tv_sec = 0;      // timeout in seconds
  response_timeout.tv_usec = 300000;     // timeout in microseconds
#if LIBMODBUS_VERSION_MAJOR > 3 \
    || (LIBMODBUS_VERSION_MAJOR == 3 && LIBMODBUS_VERSION_MINOR >= 1)
  modbus_set_response_timeout(this->mb, response_timeout.tv_sec, response_timeout.tv_usec);
#else
  modbus_set_response_timeout(this->mb, &response_timeout);
#endif

  machineState = IDLE;
  
  if(modbus_connect(this->mb) == -1) {
    machineState = DISCONNECTED;
  }
}

MPS::~MPS() {
  free(ip);
  machineState = DISCONNECTED;
  modbus_close(this->mb);
  modbus_free(this->mb);
}

void MPS::reconnect() {
  this->mb = modbus_new_tcp(this->ip, this->port);

  // set new timeout
  struct timeval response_timeout;
  response_timeout.tv_sec = 5;      // timeout in seconds
  response_timeout.tv_usec = 0;     // timeout in microseconds
#if LIBMODBUS_VERSION_MAJOR > 3 \
    || (LIBMODBUS_VERSION_MAJOR == 3 && LIBMODBUS_VERSION_MINOR >= 1)
  modbus_set_response_timeout(this->mb, response_timeout.tv_sec, response_timeout.tv_usec);
#else
  modbus_set_response_timeout(this->mb, &response_timeout);
#endif

  machineState = IDLE;
  
  if(modbus_connect(this->mb) == -1) {
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
  int rc = modbus_write_registers(mb, 10, 1, send);
  
  if(rc == -1) {
    machineState = DISCONNECTED;
  }
}

void MPS::resetMachine() {
  uint16_t send[1] = {(uint16_t) 10};

  int rc = modbus_write_registers(mb, 0, 1, send);

  if(rc == -1) {
    machineState = DISCONNECTED;
  }
}

/* lights[0] is the green light
   lights[1] is the yellow light
   lights[2] is the red light

   Each light can have 3 values.
   0 = light is off
   1 = light is on
   2 = light blinks
*/
void
MPS::setAllLights(int lights[3]) {
  uint16_t send[3] = {(uint16_t) lights[0], (uint16_t) lights[1], (uint16_t) lights[2]};

  int rc = modbus_write_registers(mb, 3, 3, send);

  if(rc == -1) {
    machineState = DISCONNECTED;
  }
}
