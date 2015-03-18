/*!
* \file mps_incoming_station.h
* \brief Definitions for IncomingStation communication
* \author David Masternak
* \version 1.0
*/

#include <mps_incoming_station.h>

/*!
* \fn MPSIncomingStation(MPSRefboxInterface* cli, int addr)
* \brief Constructor
*/
MPSIncomingStation::MPSIncomingStation(char* ip, int port) {
  this->ip = ip;
  this->port = port;

  this->mb = modbus_new_tcp(this->ip, this->port);

  if(modbus_connect(this->mb) == -1) {
    std::cout << "Error while connecting" << std::endl;
    modbus_free(this->mb);
    return -1;
  }
}

/*!
* \fn ~MPSIncomingStation()
* \brief Destructor
*/
MPSIncomingStation::~MPSIncomingStation() {

}

/*!
* \fn getCap(int color, int side)
* \brief send command to get a cap
* \param color what kind of workpiece
* \param side on which side workpiece have to be delivere
*/
void MPSIncomingStation::getCap(int color, int side) {
  uint16_t send[3] = {(uint16_t)2, (uint16_t)color, (uint16_t)side};
  
  int rc = modbus_write_registers(this->ctx, this->addr, 1, send);
  
  if(rc == -1) {
    std::cout << "ERROR while sending data" << std::endl;
  }
}

/*!
* \fn capReady()
* \brief receive capReady command
* \param empty received data
* \return true if cap is ready and false if not
*/
bool MPSIncomingStation::capReady(bool ready) {
  uint16_t reci[1] = {0};
  
  int rc = modbus_read_registers(this->ctx, this->addr, 1, reci);

  if(reci[0] == 1) {
    return true;
  }

  return false;
}

/*!
* \fn isEmpty()
* \brief receive isEmpty command
* \param empty received empty command
* \param color what color is empty
* \return true if empty and false if not
*/
bool MPSIncomingStation::isEmpty(bool empty, int color) {
  uint16_t reci[1] = {0};
  
  int rc = modbus_read_registers(this->ctx, this->addr, 1, reci);

  if(reci[0] == 1) {
    return true;
  }
  
  return false;
}
