/*!
* \file mps_deliver.cpp
* \brief implementation for Deliver communication
* \author David Masternak
* \version 1.0
*/

#include "mps_deliver.h"
#include <modbus/modbus.h>
#include <iostream>
#include <core/exception.h>
//#include "mps_deliver_message.h"

/*!
 * \fn MPSDeliver(const char* ip, int port)
 * \brief Constructor
 * \param ip address of mps
 * \param port port for modbus communication
 */
MPSDeliver::MPSDeliver(const char* ip, int port)
  : MPS(ip, port),
    Thread("default", OPMODE_WAITFORWAKEUP)
{
  type = 4;
  setType();
  machineState = DISCONNECTED;
}

MPSDeliver::MPSDeliver(const char* ip, int port, const char* name)
  : MPS(ip, port),
    Thread(name, OPMODE_WAITFORWAKEUP)
{
  type = 4;
  setType();
  machineState = DISCONNECTED;
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
    machineState = DISCONNECTED;
  }
}

/*!
 * \fn isDelivered()
 * \brief receive isDelivered data
 * \return true, if delivery was successful and false if not
 */
int MPSDeliver::isDelivered() {
  uint16_t rec[1];
  int rc = modbus_read_input_registers(mb, 5, 1, rec);

  if(rc == -1) {
    machineState = DISCONNECTED;
  }
  
  if(rec[0] == 1) {
    return 1;
  }
  return 0;
}

/*!
 * \fn processQueue()
 * \brief processing the queue
 */
// void MPSDeliver::processQueue() {
//   if(!this->messages.empty() && !this->lock) {
//     this->lock = true;
//     MPSDeliverSideMessage* tmp = (MPSDeliverSideMessage*)this->messages.front();
//     this->messages.pop();
//     std::cout << tmp->getSide() << std::endl;
//     this->sendDeliver(tmp->getSide());
//   }
//   else if(this->isDelivered()) {
//     this->lock = false;
//   }
// }

/*!
 * \fn setLight(int light, int state);
 * \param light what color
 * \param state on or off
 */
void MPSDeliver::setLight(int light, int state, int blink) {
  int rc;
  uint16_t send[1] = {(uint16_t)state};
  uint16_t sendblink[1] = {(uint16_t)blink};
  
  if(light == 1) {
    rc = modbus_write_registers(mb, 3, 1, send);
    rc = modbus_write_registers(mb, 6, 1, sendblink);
  }
  else if(light == 2) {
    rc = modbus_write_registers(mb, 4, 1, send);
    rc = modbus_write_registers(mb, 7, 1, sendblink);
  }
  else if(light == 3) {
    rc = modbus_write_registers(mb, 5, 1, send);
    rc = modbus_write_registers(mb, 8, 1, sendblink);
  }
  else {
    throw fawkes::Exception("Light state not available");
  }

  if(rc == -1) {
    machineState = DISCONNECTED;
  }
}

void
MPSDeliver::deliverProduct()
{
}


void MPSDeliver::clearRegister() {
  uint16_t send[3] = {0};
  
  int rc = modbus_write_registers(mb, 0, 3, send);

  if(rc == -1) {
    machineState = DISCONNECTED;
  }
}

MPSDeliver::MachineState
MPSDeliver::getState()
{
  uint16_t rec[1] = {0};

  int rc = modbus_read_input_registers(mb, 3, 1, rec);

  if(rc == -1) {
    machineState = DISCONNECTED;
    return DISCONNECTED;
  }

  if(rec[0] == 1) {
    machineState = AVAILABLE;
    return AVAILABLE;
  }
  else if(rec[0] == 2) {
    machineState = PROCESSING;
    return PROCESSING;
  }
  else if(rec[0] == 3) {
    machineState = PROCESSED;
    return PROCESSED;
  }
  else {
    machineState = IDLE;
    return IDLE;
  }
}

void MPSDeliver::loop() {
  getState();
}
