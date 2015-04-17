/*!
* \file mps_incoming_station.h
* \brief Definitions for IncomingStation communication
* \author David Masternak
* \version 1.0
*/

#include "mps_incoming_station.h"
//#include "mps_incoming_station_message.h"
#include <iostream>

/*!
* \fn MPSIncomingStation(const char* ip, int port)
* \param ip ip address of mps
* \param port port of modbus communication
* \brief Constructor
*/
MPSIncomingStation::MPSIncomingStation(char* ip, int port) : MPS(ip, port), Thread("test") {
  type = 1;
  this->lastId = 2;
}

/*!
 * \fn ~MPSIncomingStation()
 * \brief Destructor
 */
MPSIncomingStation::~MPSIncomingStation() {}

/*!
* \fn getCap(int color, int side)
* \brief send command to get a cap
* \param color what kind of workpiece
* \param side on which side workpiece have to be delivere
*/
void MPSIncomingStation::getCap(int color, int side) {
  uint16_t send[3] = {(uint16_t)this->lastId, (uint16_t)color, (uint16_t)side};
  std::cout << send[0] << " " << color << " " << side << std::endl;
  int rc = modbus_write_registers(mb, 0, 3, send);

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }

  this->lastId++;
}

/*!
* \fn capReady()
* \brief receive capReady command
* \return true if cap is ready and false if not
*/
bool MPSIncomingStation::capReady() {
  uint16_t rec[1] = {0};
  
  int rc = modbus_read_input_registers(mb, 0, 1, rec);

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }
  
  if(rec[0] == 13) {
    std::cout << "here" << std::endl;
    return true;
  }

  return false;
}

/*!
* \fn isEmpty()
* \brief receive isEmpty command
* \return true if empty and false if not
*/
int MPSIncomingStation::isEmpty() {
  uint16_t rec[1] = {0};
  
  int rc = modbus_read_input_registers(mb, 1, 1, rec);

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }
  
  return rec[0];
}
  
/*!
 * \fn processQueue()
 * \brief processing the queue
 */
// void MPSIncomingStation::processQueue() {
//   if(!this->messages.empty() && !this->lock) {
//     this->lock = true;
//     MPSIncomingStationGiveCapMessage* tmp = (MPSIncomingStationGiveCapMessage*)messages.front();
//     messages.pop();

//     this->getCap(tmp->getColor(), tmp->getSide());
//   }
//   else if(this->capReady()) {
//     this->lock = false;
//   }
// }

/*!
 * \fn setLight(int light, int state);
 * \param light what color
 * \param state on or off
 */
void MPSIncomingStation::setLight(int light, int state, int blink) {
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
    std::cout << "Light not available" << std::endl;
  }

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }
}

/*!
 * \fn clearRegister();
 * \brief set register to 0
 */
void MPSIncomingStation::clearRegister() {
  uint16_t send[3] = {0};
  
  int rc = modbus_write_registers(mb, 0, 9, send);

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }
}

/*!
 * \fn clearRegister();
 * \brief set all registeres to 0
 */
MPSIncomingStation::MachineState MPSIncomingStation::getState() {
  uint16_t rec[1] = {0};
  
  int rc = modbus_read_input_registers(mb, 3, 1, rec);

  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }

  if(rec[0] == 1) {
    return PROCESSING;
  }
  else if(rec[0] == 2) {
    return DELIVERED;
  }
  else if(rec[0] == 3) {
    return RETRIEVED;
  }
  else {
    return IDLE;
  }
}
