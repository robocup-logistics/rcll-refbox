/*!
* \file mps.h
* \brief Definition of a mps machine
* \author David Masternak
* \version 1.0
*/

#ifndef MPS_H
#define MPS_H

#define MODBUS_TCP_SLAVE 0x04

#include <queue>
#include <modbus/modbus.h>
#include <vector>

#include "mps_message.h"

/*!
* \class MPS
* \brief Abstract definition of a mps
*/
class MPS {
  //private:
 
 protected:
  bool lock; // mps is locked because it process something
  std::queue<MPSMessage*> messages;
  const char* ip;
  int port;
  int status;
  modbus_t* mb;

 public:
  /*!
   * \fn MPS(const char* ip, int port)
   * \param ip ip address of machine
   * \param port port of modbus communication
   * \brief Constructor
   */
  MPS(const char* ip, int port);

  /*!
   * \fn ~MPS()
   * \brief Destructor
   */
  ~MPS();
  
  /*!
   * \fn isLocked()
   * \brief getter for lock
   * \return true if mps is locked and false if not
   */
  bool isLocked() {
    return this->lock;
  }
  
  /*!
   * \fn setLocked(bool lock)
   * \brief setter for lock
   * \param lock true if mps is locked and false if not
   */
  void setLock(bool lock) {
    this->lock = lock;
  }

  /*!
   * \fn setMessage(MPSMessage* message)
   * \brief setter for lock
   * \param lock true if mps is locked and false if not
   */
  void setMessage(MPSMessage* message) {
    this->messages.push(message);
  }
};

#endif // MPS_H
