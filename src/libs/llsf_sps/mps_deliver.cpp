/*!
* \file mps_deliver.cpp
* \brief implementation for Deliver communication
* \author David Masternak
* \version 1.0
*/

#include "mps_deliver.h"
#include <modbus/modbus.h>
#include <iostream>
#include "mps_deliver_message.h"

/*!
 * \fn MPSDeliver(const char* ip, int port)
 * \brief Constructor
 * \param ip address of mps
 * \param port port for modbus communication
 */
MPSDeliver::MPSDeliver(const char* ip, int port) : MPS(ip, port) {}

/*!
 * \fn sendDeliver(int lane)
 * \brief send deliver command
 * \param lane on which lane have to be deliver
 */
void MPSDeliver::sendDeliver(int lane) {
  uint16_t send[1] = {(uint16_t)lane};
  int rc = modbus_write_registers(mb, 1, 1, send);
  
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
  int rc = modbus_read_input_registers(mb, 2, 1, rec);

  if(rec[0] == 0) {
    return false;
  }
  else {
    return true;
  }
}

/*!
 * \fn processQueue()
 * \brief processing the queue
 */
void MPSDeliver::processQueue() {
  if(!this->messages.empty() && !this->lock) {
    this->lock = true;
    MPSDeliverSideMessage* tmp = (MPSDeliverSideMessage*)this->messages.front();
    this->messages.pop();
    std::cout << tmp->getSide() << std::endl;
    this->sendDeliver(tmp->getSide());
  }
  else if(this->isDelivered()) {
    this->lock = false;
  }
}
