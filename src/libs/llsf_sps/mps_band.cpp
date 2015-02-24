/*!
* \file mps_band.cpp
* \brief Implementation for communication between Refbox and Band
* \author David Masternak
* \version 1.0
*/

#include <mps_band.h>

#include <iostream>

/*!
 * \fn MPSBand(MPSRefboxInterface* cli, int addr)
 * \brief Constructor
 * \param cli reference of Refbox Interface
 * \param addr address of destination MPS
 */
MPSBand::MPSBand(MPSRefboxInterface *cli, int addr) {
  this->ctx = cli->getTcpConnection();
  this->addr = addr;
}

/*!
 * \fn run()
 * \brief sending the command to run the band
 */
void MPSBand::runBand() {
  uint16_t send[1] = {(uint16_t)2};
  
  int rc = modbus_write_registers(this->ctx, this->addr, 1, send);
  
  if(rc == -1) {
    std::cout << "ERROR while sending data" << std::endl;
  }
}

/*!
 * \fn isReady()
 * \brief receive isReady command
 * \param received data
 * \return true if workpiece is ready and false if not
 */
bool MPSBand::isReady(bool ready) {
  return ready;
}

void MPSBand::receiveData() {
  uint16_t reci[2] = {0};
  
  int rc = modbus_read_registers(this->ctx, this->addr, 2, reci);

  int command = (int)reci[0];

  switch(command) {
  case 3:
    this->isReady((bool)reci[1]);
    break;
  default:
    std::cout << "Unkown message" << std::endl;
    break;
  }
}
