/*!
* \file mps_incoming_station.h
* \brief Definitions for IncomingStation communication
* \author David Masternak
* \version 1.0
*/

#include "mps_incoming_station.h"
#include "mps_incoming_station_message.h"
#include <iostream>

/*!
* \fn MPSIncomingStation(const char* ip, int port)
* \param ip ip address of mps
* \param port port of modbus communication
* \brief Constructor
*/
MPSIncomingStation::MPSIncomingStation(const char* ip, int port) : MPS(ip, port){}

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
  uint16_t send[2] = {(uint16_t)color, (uint16_t)side};
  
  int rc = modbus_write_registers(this->mb, 1, 2, send);
  
  if(rc == -1) {
    std::cout << "ERROR while sending data with ip: " << ip << std::endl;
  }
}

/*!
* \fn capReady()
* \brief receive capReady command
* \return true if cap is ready and false if not
*/
bool MPSIncomingStation::capReady() {
  uint16_t rec[1] = {0};
  
  int rc = modbus_read_input_registers(this->mb, 3, 1, rec);

  if(rec[0] == 1) {
    return true;
  }

  return false;
}

/*!
* \fn isEmpty()
* \brief receive isEmpty command
* \return true if empty and false if not
*/
bool MPSIncomingStation::isEmpty() {
  uint16_t rec[1] = {0};
  
  int rc = modbus_read_input_registers(this->mb, 4, 1, rec);

  if(rec[0] == 1) {
    return true;
  }
  
  return false;
}

/*!
 * \fn processQueue()
 * \brief processing the queue
 */
void MPSIncomingStation::processQueue() {
  if(!this->messages.empty() && !this->lock) {
    this->lock = true;
    MPSIncomingStationGiveCapMessage* tmp = (MPSIncomingStationGiveCapMessage*)messages.front();
    messages.pop();

    this->getCap(tmp->getColor(), tmp->getSide());
  }
  else if(this->capReady()) {
    this->lock = false;
  }
}
