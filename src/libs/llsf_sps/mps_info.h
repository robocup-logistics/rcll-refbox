/*!
* \file mps_info.h
* \brief Data from a MPS
* \author David Masternak
* \version 1.0
*/

#ifndef MPSINFO_H
#define MPSINFO_H

#include "mps.h"

/*!
* \class MPS
* \brief Datastruct of MPS information
*/
class MPSInfo {
 private:
  const char *ip; // ip of a MPS
  int socket; // socket/file descriptor of a MPS
  int address; // address of MPS
  MPS* machine; // Reference to machine

 public:
  /*!
   * \fn MPS()
   * \brief Constructor
   */
  MPSInfo();

  /*!
   * \fn MPS(char* ip, int socket)
   * \brief Constructor
   * \param ip ip of MPS
   * \param socket socket/file descriptor
   */
  MPSInfo(char* ip, int socket);

  /*!
   * \fn setIp(char* ip)
   * \brief setter for ip
   * \param ip ip of MPS
   */
  void setIp(char* ip);

  /*!
   * \fn setSocket(int socket)
   * \brief setter for socket/file descriptor
   * \param socket socket/file descriptor of MPS
   */
  void setSocket(int socket);

  /*!
   * \fn setAddress(int address)
   * \brief setter for address
   * \param address address of MPS
   */
  void setAddress(int address);

  /*!
   * \fn setMachine(MPS mps)
   * \brief setter for machine
   * \param address reference to machine
   */
  void setMachine(MPS* mps);

  /*!
   * \fn getIp()
   * \brief getter for address
   * \return ip of MPS
   */
  const char* getIp();

  /*!
   * \fn getSocket()
   * \brief getter for socket/file descriptor
   * \return socket/file descriptor of MPS
   */
  int getSocket();

  /*!
   * \fn getAddress()
   * \brief getter for address
   * \return socket of MPS
   */
  int getAddress();

  /*!
   * \fn getMachine()
   * \brief getter for machine
   * \return reference of machine
   */
  MPS* getMachine();
};

#endif // MPSINFO_H
