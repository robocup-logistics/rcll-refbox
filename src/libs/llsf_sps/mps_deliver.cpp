/*!
* \file mps_deliver.cpp
* \brief implementation for Deliver communication
* \author David Masternak
* \version 1.0
*/

#include <mps_deliver.h>
#include <modbus/modbus.h>

/*!
 * \fn MPSDeliver(MPSRefboxInterface* cli, int addr)
 * \brief Constructor
 * \param cli reference of Refbox Interface
 * \param port port for modbus communication
 */
MPSDeliver::MPSDeliver(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  if(modbus_connect(this->mb) == -1) {
    std::cout << "Error while connecting" << std::endl;
    modbus_free(mb);
    return -1;
  }
}

/*!
 * \fn sendDeliver(int lane)
 * \brief send deliver command
 * \param lane on which lane have to be deliver
 */
void MPSDeliver::sendDeliver(int lane) {
  uint16_t send[2] = {2, lane};
  
  int rc = modbus_write_registers(this->ctx, this->addr, 2, send);
  
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
