/*!
* \file mps_deliver.cpp
* \brief implementation for Deliver communication
* \author David Masternak
* \version 1.0
*/

#include "mps_deliver.h"
#include <modbus/modbus.h>
#include <iostream>

/*!
 * \fn MPSDeliver(char* ip, int port)
 * \brief Constructor
 * \param ip address of mps
 * \param port port for modbus communication
 */
MPSDeliver::MPSDeliver(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  if(modbus_connect(this->mb) == -1) {
    std::cout << "Error while connecting" << std::endl;
    modbus_free(mb);
  }
}

/*!
 * \fn ~MPSDeliver()
 * \brief Destructor
 */
MPSDeliver::~MPSDeliver() {
  modbus_close(this->mb);
  modbus_free(this->mb);
}

/*!
 * \fn sendDeliver(int lane)
 * \brief send deliver command
 * \param lane on which lane have to be deliver
 */
void MPSDeliver::sendDeliver(int lane) {
  uint16_t send[1] = {(uint16_t)lane};
  int rc = modbus_write_registers(this->mb, 1, 1, send);
  
  if(rc == -1) {
    std::cout << "ERROR while sending data" << std::endl;
  }
}

/*!
 * \fn isDelivered()
 * \brief receive isDelivered data
 * \return true, if delivery was successful and false if not
 */
bool MPSDeliver::isDelivered() {
  uint16_t rec[1];
  int rc = modbus_read_input_registers(this->mb, 2, 1, rec);

  if(rec[0] == 0) {
    return false;
  }
  else {
    return true;
  }
}
