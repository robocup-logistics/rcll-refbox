/*!
* \file mps_pick_place_2.cpp
* \brief Implementation of communication between Refbox and Pick&Place 1
* \author David Masternak
* \version 1.0
*/

#include <mps_pick_place_2.h>

/*!
* \fn MPSPickPlace1(MPSRefboxInterface *cli, int addr)
* \brief Constructor
* \param *cli A reference to refboxinterface
* \param addr The address of destination MPS
*/

MPSPickPlace2::MPSPickPlace2(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  if(modbus_connect(this->mb) == -1) {
    std::cout << "Error while connecting" << std::endl;
    modbus_free(this->mb);
    return -1;
  }
}

/*!
* \fn produceRing(int workpiece)
* \brief send produce message over Modbus
* \param workpiece what kind of workpiece have to be produce
*/
void MPSPickPlace2::produceRing(int workpiece) {
  uint16_t send[4] = {2, 4, 8, 16};
  
  int rc = modbus_write_registers(this->ctx, this->addr, 4, send);
  
  if(rc == -1) {
    std::cout << "ERROR while sending data" << std::endl;
  }
}

/*!
* \fn ringReady()
* \brief receive ringReady command
* \param ready received ringReady command
* \return true if ring is ready and false if ring not is ready
*/
bool MPSPickPlace2::ringReady(bool ready) {
  return ready;
}

/*!
* \fn isEmptyLine1()
* \brief receive isEmptyLine1 command
* \return true if line 1 is empty
*/
bool MPSPickPlace2::isEmpty(int lane, bool empty) {
  return empty;
}
