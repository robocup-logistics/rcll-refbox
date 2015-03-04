/*!
* \file mps_pick_place_1.cpp
* \brief Implementation of communication between Refbox and Pick&Place 1
* \author David Masternak
* \version 1.0
*/

#include <mps_pick_place_1.h>

#include <iostream>

/*!
* \fn MPSPickPlace1(MPSRefboxInterface *cli, int addr)
* \brief Constructor
* \param *cli A reference to refboxinterface
* \param addr The address of destination MPS
*/
MPSPickPlace1::MPSPickPlace1(MPSRefboxInterface *cli, int addr) {
  this->ctx = cli->getTcpConnection();
  this->addr = addr;
}

/*!
* \fn produceEnd()
* \brief send produce message over Modbus
*/
void MPSPickPlace1::produceEnd() {
  int nb = 1;

  // uint16_t *tab_rq_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
  // memset(tab_rq_registers, 0, nb * sizeof(uint16_t));

  uint16_t send[1] = {5};
  uint16_t reci[20] = {0};

  //tab_rq_registers[0] = (uint8_t) 5;

  /* SINGLE REGISTER */
  std::cout << "Write Data" << std::endl;
  int rc = modbus_write_register(ctx, addr, send[0]);
	
  if (rc != 1) {
    std::cout << "ERROR while writing data to address " << 5 << std::endl;
    rc = modbus_read_registers(ctx, 5, 1, reci);
  } 
}

/*!
* \fn isEmpty()
* \brief receive isempty message from MPS
* \param empty received isEmpty command
* \return ture, if MPS is empty or false, if MPS is not empty
*/
bool MPSPickPlace1::isEmpty(bool empty) {
  uint16_t reci[20] = {0};

  int rc = modbus_read_registers(ctx, this->addr, 1, reci);

  if(rc != 1) {
    std::cout << "ERROR while reading data from address " << this->addr << std::endl;
  }

  std::cout << "what up: " << reci[0] << std::endl;

  if(reci[0] == 1) {
    return true;
  }
  
  return false;
}

/*!
* \fn isEmptyReady()
* \brief receive isready message from MPS
* \param reveiced isReady command
* \return true, if MPS is ready and false, if MPS is not ready
*/
bool MPSPickPlace1::isReady(bool ready) {
  uint16_t reci[20] = {0};

  int rc = modbus_read_registers(ctx, this->addr, 1, reci);

  if(rc != 1) {
    std::cout << "ERROR while reading data from address " << this->addr << std::endl;
  }

  if(reci[0] == 1) {
    return true;
  }

  return false;
}

/*!
 * \fn receiveData()
 * \brief receive data from MPS and capsulate this data into the MPSMessage datastruct.
 */
void MPSPickPlace1::receiveData() {
  // 3 isEmpty, 4 isReady
  uint16_t reci[2] = {0};
  
  int rc = modbus_read_registers(this->ctx, this->addr, 2, reci);

  int command = (int)reci[0];

  switch(command) {
  case 3:
    this->isEmpty((bool)reci[1]);
    break;
  case 4:
    this->isReady((bool)reci[1]);
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
void MPSPickPlace1::sendData() {

}
