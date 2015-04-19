/*!
* \file mps_pick_place_2.cpp
* \brief Implementation of communication between Refbox and Pick&Place 1
* \author David Masternak
* \version 1.0
*/

#include "mps_pick_place_2.h"
//#include "mps_pick_place_2_message.h"
#include <iostream>

#include <core/exception.h>

/*!
* \fn MPSPickPlace1(const char* ip, int port)
* \brief Constructor
* \param ip address of mps
* \param port port of modbus communication
*/
MPSPickPlace2::MPSPickPlace2(char* ip, int port) : MPS(ip, port), Thread("default", OPMODE_WAITFORWAKEUP) {
  type = 3;
}

MPSPickPlace2::MPSPickPlace2(char* ip, int port, const char* name) : MPS(ip, port), Thread(name, OPMODE_WAITFORWAKEUP) {
  type = 3;
}

/*!
 * \fn ~MPSPickPlace2()
 * \brief Destructor
 */
MPSPickPlace2::~MPSPickPlace2() {}

/*!
* \fn produceRing(int workpiece)
* \brief send produce message over Modbus
* \param workpiece what kind of workpiece have to be produce
*/
void MPSPickPlace2::produceRing(int workpiece) {
  uint16_t send[1] = {(uint16_t)workpiece};
  int rc = modbus_write_registers(mb, 0, 1, send);
  
  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }
}

void MPSPickPlace2::deliverProduct() {
  uint16_t send[1] = {(uint16_t) 1};
  int rc = modbus_write_registers(mb, 1, 1, send);
  
  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }
}

/*!
* \fn ringReady()
* \brief receive ringReady command
* \return true if ring is ready and false if ring not is ready
*/
bool MPSPickPlace2::ringReady() {
  uint16_t rec[1];
  int rc = modbus_read_input_registers(mb, 0, 1, rec);

  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }

  if(rec[0] == 13) {
    return true;
  }
  else {
    return false;
  }
}

/*!
* \fn isEmptyLine()
* \brief receive isEmptyLine1 command
* \return true if line 1 is empty
*/
bool MPSPickPlace2::isEmpty() {
  uint16_t rec[1];
  int rc = modbus_read_input_registers(mb, 3, 1, rec);

  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }

  if(rec[0] == 1) {
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
// void MPSPickPlace2::processQueue() {
//   if(!this->messages.empty() && !this->lock) {
//     this->lock = true;
//     MPSPickPlace2ProduceRingMessage* tmp = (MPSPickPlace2ProduceRingMessage*)messages.front();
//     messages.pop();

//     this->produceRing(tmp->getRing());
//   }
//   else if(this->ringReady()) {
//     this->lock = false;
//   }
// }

/*!
 * \fn setLight(int light, int state);
 * \param light what color
 * \param state on or off
 */
void MPSPickPlace2::setLight(int light, int state, int blink) {
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
    throw fawkes::Exception("Cannot set light state");
  }

  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }
}

void MPSPickPlace2::clearRegister() {
  uint16_t send[3] = {0};
  
  int rc = modbus_write_registers(mb, 0, 3, send);

  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }
}

MPSPickPlace2::MachineState MPSPickPlace2::getState() {
  uint16_t rec[1] = {0};

  int rc = modbus_read_input_registers(mb, 3, 1, rec);

  if(rc != 1) {
    state = NOTSENDRECEIVED;
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
  else if(rec[0] == 4) {
    machineState = DELIVER;
    return DELIVER;
  }
  else if(rec[0] == 5) {
    machineState = DELIVERED;
    return DELIVERED;
  }
  else if(rec[0] == 6) {
    machineState = RETRIEVED;
    return RETRIEVED;
  }
  else {
    return IDLE;
  }
}

void MPSPickPlace2::loop() {
  getState();
}
