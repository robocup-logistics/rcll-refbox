#include "mps.h"
#include <iostream>

MPS::MPS(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  if(modbus_connect(this->mb) == -1) {
    state = DISCONNECTED;
  }
  else {
    state = CONNECTED;
  }
}

MPS::~MPS() {
  state = DISCONNECTED;
  modbus_close(this->mb);
  modbus_free(this->mb);
}

void MPS::reconnect() {
  modbus_close(mb);
  if (modbus_connect(mb) == -1) {
    state = DISCONNECTED;
  }
  else {
    state = CONNECTED;
  }
}
