#include "mps.h"
#include <iostream>

MPS::MPS(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  std::cout << this->mb << std::endl;
  
  if(modbus_connect(this->mb) == -1) {
    std::cout << "Error while connecting with ip: " << ip << std::endl;
  }
}

MPS::~MPS() {
  std::cout << "Destructor called" << std::endl;
  modbus_close(this->mb);
  modbus_free(this->mb);
}
