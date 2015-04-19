/*!
* \file mps.h
* \brief Definition of a mps machine
* \author David Masternak
* \version 1.0
*/

#ifndef MPS_H
#define MPS_H

//#define MODBUS_TCP_SLAVE 0x04

#include <queue>
#include <modbus/modbus.h>
#include <vector>
#include <string>

#include "mps_message.h"

/*!
* \class MPS
* \brief Abstract definition of a mps
*/
class MPS {
  //private:
 public:
  enum MachineState {IDLE, AVAILABLE, PROCESSING, PROCESSED, DELIVER, DELIVERED, RETRIEVED, DISCONNECTED};
  MachineState machineState;
  
 protected:
  bool lock; // mps is locked because it process something
  std::queue<MPSMessage*> messages;
  char* ip;
  int port;
  int status;
  modbus_t* mb;
  int type;
  
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
  virtual ~MPS();

  void reconnect();
  
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

  char* getIp() {
    return this->ip;
  };

  int getType() {
    return this->type;
  };

  virtual std::string machienStateString();

  /*!
   * \fn setLight(int light, int state);
   * \param light what color
   * \param state on or off
   */
  virtual void setLight(int light, int state, int blink) = 0;


  virtual void clearRegister() = 0;

  virtual void deliverProduct() = 0;

};

#endif // MPS_H
