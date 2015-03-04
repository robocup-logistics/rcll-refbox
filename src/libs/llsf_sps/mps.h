/*!
* \file mps.h
* \brief Definition of a mps machine
* \author David Masternak
* \version 1.0
*/

#ifndef MPS_H
#define MPS_H

#include <queue>
#include <mps_message.h>
#include <modbus/modbus.h>

/*!
* \class MPS
* \brief Abstract definition of a mps
*/
class MPS {
 private:
  bool lock; // mps is locked because it process something

 public:
  priority_queue<MPSMessage> messageQueue; // queue that holds all messages for this mps

  /*!
   * \fn receiveData()
   * \brief receive data from MPS and capsulate this data into the MPSMessage datastruct.
   */
  virtual void receiveData() = 0;

  /*!
   * \fn sendData()
   * \brief write data from MPS and encapsulate this data into the modbus protocol datastruct.
   */
  virtual void sendData() = 0;

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
   * \fn capsulateMessage(int[] lock)
   * \brief capsulate a message from a incoming message
   * \param msg array of the message
   * \return new messagetype
   */
  MPSMessage* capsulateMessage(uint16_t[] msg) {
    int msgsize = sizeof(msg) / sizeof(uint16_t);

    uint16_t[msgsize - 1] rawmsg;
 
    for(int i = 2; i < msgsize; i++) {
      
    }

    MPSMessage *msg = new MPSMessage(0, 0, msg[0], msg[1]);
    return msg;
  }

  /*!
   * \fn encapsulateMessage(MPSMessage* msg)
   * \brief encapsulate a message to a outgiong message
   * \param msg the message object
   * \return array that can be send with modbus
   */
  uint16_t[] encapsulateMessage(MPSMessage* msg) {
    // at this point we need no longer the priority
    
  }
};

#endif // MPS_H
