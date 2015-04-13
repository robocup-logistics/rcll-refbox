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
MPSDeliver::MPSDeliver(const char* ip, int port) : MPS(ip, port) {
  type = 4;
}

/*!
 * \fn sendDeliver(int lane)
 * \brief send deliver command
 * \param lane on which lane have to be deliver
 */
void MPSDeliver::sendDeliver(int lane) {
  uint16_t send[1] = {(uint16_t)lane};
  int rc = modbus_write_registers(mb, 0, 1, send);
  
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
  int rc = modbus_read_input_registers(mb, 0, 1, rec);

  if(rec[0] != 13) {
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

/*!
 * \fn setLight(int light, int state);
 * \param light what color
 * \param state on or off
 */
void MPSDeliver::setLight(int light, int state) {
  int rc;
  uint16_t send[1] = {(uint16_t)state};
  
  if(light == 1) {
    rc = modbus_write_registers(mb, 3, 1, send);    
  }
  else if(light == 2) {
    rc = modbus_write_registers(mb, 4, 1, send);
  }
  else if(light == 3) {
    rc = modbus_write_registers(mb, 5, 1, send);
  }
  else {
    std::cout << "Light not available" << std::endl;
  }

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }
}
