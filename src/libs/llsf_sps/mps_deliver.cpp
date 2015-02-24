/*!
* \file mps_deliver.cpp
* \brief implementation for Deliver communication
* \author David Masternak
* \version 1.0
*/

#include<mps_deliver.h>

/*!
 * \fn MPSDeliver(MPSRefboxInterface* cli, int addr)
 * \brief Constructor
 * \param cli reference of Refbox Interface
 * \param port port for modbus communication
 */
MPSDeliver::MPSDeliver(MPSRefboxInterface* cli, int addr) {
  this->ctx = cli->getTcpConnection();
  this->addr = addr;
}

/*!
 * \fn sendDeliver(int lane)
 * \brief send deliver command
 * \param lane on which lane have to be deliver
 */
void MPSDeliver::sendDeliver(int lane) {
  uint16_t send[4] = {2, 4, 8, 16};
  
  int rc = modbus_write_registers(this->ctx, this->addr, 4, send);
  
  if(rc == -1) {
    std::cout << "ERROR while sending data" << std::endl;
  }
}

/*!
 * \fn isDelivered()
 * \brief receive isDelivered data
 * \return true, if delivery was successful and false if not
 */
bool MPSDeliver::isDelivered(bool ready) {
  if(ready == true) {
    std::cout << "true" << std::endl;
    return true;
  }
  else {
    std::cout << "false" << std::endl;
    return false;
  }
}

void MPSDeliver::receiveData() {
  uint16_t reci[2] = {0};
  
  int rc = modbus_read_registers(this->ctx, this->addr, 2, reci);

  int command = (int)reci[0];

  switch(command) {
  case 3:
    this->isDelivered((bool)reci[1]);
    break;
  default:
    std::cout << "Unknown message" << std::endl;
    break;
  }
}
