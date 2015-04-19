/*!
* \file mps_pick_place_1.cpp
* \brief Implementation of communication between Refbox and Pick&Place 1
* \author David Masternak
* \version 1.0
*/

#include "mps_pick_place_1.h"
//#include "mps_pick_place_1_message.h"

#include <iostream>

#include <core/exception.h>

/*!
* \fn MPSPickPlace1(const char* ip, int port)
* \brief Constructor
* \param ip address of mps
* \param port port of modbus communication
*/
MPSPickPlace1::MPSPickPlace1(char* ip, int port) : MPS(ip, port), Thread("test", OPMODE_WAITFORWAKEUP) {
  type = 2;
}

MPSPickPlace1::MPSPickPlace1(char* ip, int port, const char* name): MPS(ip, port), Thread(name, OPMODE_WAITFORWAKEUP) {
  type = 2;
}

/*!
 * \fn ~MPSPickPlace1()
 * \brief Destructor
 */
MPSPickPlace1::~MPSPickPlace1() {}

/*!
* \fn produceEnd(int updown)
* \param updown set workpiece(1) or get workpiece(2)
* \brief send produce message over Modbus
*/
void MPSPickPlace1::produceEnd(int updown) {
  uint16_t send1[1] = {(uint16_t)updown};
  std::cout << updown << std::endl;
  int rc = modbus_write_registers(mb, 0, 1, send1);
  	
  if (rc != 1) {
    state = DISCONNECTED;
  } 
}

void MPSPickPlace1::deliverProduct() {
  uint16_t send1[1] = {(uint16_t) 1};
  int rc = modbus_write_registers(mb, 1, 1, send1);
  	
  if (rc != 1) {
    state = NOTSENDRECEIVED;
  }
}

/*!
* \fn isEmpty()
* \brief receive isempty message from MPS
* \return ture, if MPS is empty or false, if MPS is not empty
*/
bool MPSPickPlace1::isEmpty() {
  uint16_t rec[1];
  int rc = modbus_read_input_registers(mb, 0, 1, rec);

  if(rc != 1) {
    state = DISCONNECTED;
  }

  if(rec[0] == 13) {
    return true;
  }
  
  return false;
}

/*!
* \fn isEmptyReady()
* \brief receive isready message from MPS
* \return true, if MPS is ready and false, if MPS is not ready
*/
bool MPSPickPlace1::isReady() {
  uint16_t rec[1] = {0};

  int rc = modbus_read_input_registers(mb, 3, 1, rec);

  if(rc != 1) {
    state = DISCONNECTED;
  }

  if(rec[0] == 1) {
    return true;
  }

  return false;
}

/*!
 * \fn processQueue()
 * \brief processing the queue
 */
// void MPSPickPlace1::processQueue() {
//   if(!this->messages.empty() && !this->lock) {
//     this->lock = true;
//     MPSPickPlace1ProduceEndMessage* tmp = (MPSPickPlace1ProduceEndMessage*)messages.front();
//     messages.pop();

//     this->produceEnd(tmp->getUpdown());
//   }
//   else if(isReady()) {
//     this->lock = false;
//   }
// }


/*!
 * \fn setLight(int light, int state);
 * \param light what color
 * \param state on or off
 */
void MPSPickPlace1::setLight(int light, int state, int blink) {
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
    throw fawkes::Exception("Lightstate not available");
  }

  if(rc == -1) {
    state = DISCONNECTED;
  }
}

void MPSPickPlace1::clearRegister() {
  uint16_t send[3] = {0};
  
  int rc = modbus_write_registers(mb, 0, 3, send);
 
  if(rc == -1) {
    state = NOTSENDRECEIVED;
  }
}

MPSPickPlace1::MachineState MPSPickPlace1::getState() {
  uint16_t rec[1] = {0};

  int rc = modbus_read_input_registers(mb, 3, 1, rec);

  if(rc != 1) {
    state = DISCONNECTED;
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

void MPSPickPlace1::loop() {
  getState();
}
