/*!
* \file mps_pick_place_1.cpp
* \brief Implementation of communication between Refbox and Pick&Place 1
* \author David Masternak
* \version 1.0
*/

#include "mps_pick_place_1.h"

#include <iostream>

/*!
* \fn MPSPickPlace1(MPSRefboxInterface *cli, int port)
* \brief Constructor
* \param ip address of mps
* \param port port of modbus communication
*/
MPSPickPlace1::MPSPickPlace1(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  if(modbus_connect(this->mb) == -1) {
    std::cout << "Error while connecting" << std::endl;
    modbus_free(mb);
    //return -1;
  }
}

/*!
 * \fn ~MPSPickPlace1()
 * \brief Destructor
 */
MPSPickPlace1::~MPSPickPlace1() {
  modbus_close(this->mb);
  modbus_free(this->mb);
}

/*!
* \fn produceEnd(int updown)
* \param updown set workpiece(1) or get workpiece(2)
* \brief send produce message over Modbus
*/
void MPSPickPlace1::produceEnd(int updown) {
  uint16_t send[1] = {(uint16_t)updown};
  int rc = modbus_write_registers(this->mb, 1, 1, send);
	
  if (rc != 1) {
    std::cout << "ERROR while writing data to address " << std::endl;
  } 
}

/*!
* \fn isEmpty()
* \brief receive isempty message from MPS
* \return ture, if MPS is empty or false, if MPS is not empty
*/
bool MPSPickPlace1::isEmpty() {
  uint16_t rec[1];
  int rc = modbus_read_input_registers(this->mb, 2, 1, rec);

  if(rc != 1) {
    std::cout << "ERROR while reading data from address " << std::endl;
  }

  if(rec[0] == 1) {
    return true;
  }
  
  return false;
}

/*!
* \fn isEmptyReady()
* \brief receive isready message from MPS
* \return true, if MPS is ready and false, if MPS is not ready
*/
bool MPSPickPlace1::isReady() {
  uint16_t rec[1] = {0};

  int rc = modbus_read_input_registers(this->mb, 3, 1, rec);

  if(rc != 1) {
    std::cout << "ERROR while reading data from address " << std::endl;
  }

  if(rec[0] == 1) {
    return true;
  }

  return false;
}
