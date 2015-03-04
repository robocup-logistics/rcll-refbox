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

MPSPickPlace2::MPSPickPlace2(MPSRefboxInterface* cli, int addr) {
  this->ctx = cli->getTcpConnection();
  this->addr = addr;
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


/*!
 * \fn receiveData()
 * \brief receive data from MPS and capsulate this data into the MPSMessage datastruct.
 */
void MPSPickPlace2::receiveData() {
 // 3 isEmpty, 4 isReady
  uint16_t reci[3] = {0};
  
  int rc = modbus_read_registers(this->ctx, this->addr, 3, reci);

  int command = (int)reci[0];

  switch(command) {
  case 3:
    this->isEmpty((bool)reci[1], (int)reci[2]);
    break;
  case 4:
    this->ringReady((bool)reci[1]);
    break;
  default:
    std::cout << "Unkown message" << std::endl;
    break;
  }
}

/*!
 * \fn sendData()
 * \brief write data from MPS and encapsulate this data into the modbus protocol datastruct.
 */
void MPSPickPlace2::sendData() {

}
