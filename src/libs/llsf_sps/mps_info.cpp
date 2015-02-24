/*!
* \file mps_info.cpp
* \brief Data from a MPS
* \author David Masternak
* \version 1.0
*/

#include <mps_info.h>

/*!
 * \fn MPSInfo()
 * \brief Constructor
 */
MPSInfo::MPSInfo() {
  this->ip = "127.0.0.1";
  this->socket = 0;
  this->address = 0;
  this->machine = 0;
}

/*!
 * \fn MPSInfo(char* ip, int socket)
 * \brief Constructor
 * \param ip ip of MPS
 * \param socket socket/file descriptor
 */
MPSInfo::MPSInfo(char* ip, int socket) {
  this->ip = ip;
  this->socket = socket;
  this->address = 0;
  this->machine = 0;
}

/*!
 * \fn setIp(char* ip)
 * \brief setter for ip
 * \param ip ip of MPS
 */
void MPSInfo::setIp(char* ip) {
  this->ip = ip;
}

/*!
 * \fn setSocket(int socket)
 * \brief setter for socket/file descriptor
 * \param socket socket/file descriptor of MPS
 */
void MPSInfo::setSocket(int socket) {
  this->socket = socket;
}

/*!
 * \fn setAddress(int address)
 * \brief setter for address
 * \param address address of MPS
 */
void MPSInfo::setAddress(int address) {
  this->address = address;
}

/*!
 * \fn setMachine(MPS mps)
 * \brief setter for machine
 * \param address reference to machine
 */
void MPSInfo::setMachine(MPS* mps) {
  this->machine = mps;
}

/*!
 * \fn getIp()
 * \brief getter for address
 * \return ip ip of MPS
 */
char* MPSInfo::getIp() {
  return this->ip;
}

/*!
 * \fn getSocket()
 * \brief setter for socket/file descriptor
 * \return socket socket/file descriptor of MPS
 */
int MPSInfo::getSocket() {
  return this->socket;
}

/*!
 * \fn getAddress()
 * \brief setter for address
 * \return address socket of MPS
 */
int MPSInfo::getAddress() {
  return this->address;
}

/*!
 * \fn getMachine()
 * \brief getter for machine
 * \return reference of machine
 */
MPS* MPSInfo::getMachine() {
  return this->machine;
}
