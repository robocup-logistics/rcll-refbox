/*!
* \file mps_pick_place_1.h
* \brief Definition of communication between Refbox and Pick&Place 1
* \author David Masternak
* \version 1.0
*/

#ifndef MPSPICKPLACE1_H
#define MPSPICKPLACE1_H

#include "mps.h"

/*!
* \class MPSPickPlace1
* \brief This class handels the communication to a Pick&Place 1
*/
class MPSPickPlace1 : public MPS {
 public:
  /*!
   * \fn MPSPickPlace1(char* ip, int port)
   * \brief Initialize communication
   * \param ip address of mps
   * \param port port of modbus communication
   */
  MPSPickPlace1(const char* ip, int port);

  /*!
   * \fn ~MPSPickPlace1()
   * \brief Destructor
   */
  ~MPSPickPlace1();
  
  /*!
   * \fn produceEnd(int updown)
   * \param updown set workpiece(1) or get workpiece(2)
   * \brief send command to produce workpiece
   */
  void produceEnd(int updown);

  /*!
   * \fn isEmpty()
   * \brief receive isEmpty command
   * \return true, if lane is empty or false, if lane is not empty
   */
  bool isEmpty();

  /*!
   * \fn isReady()
   * \brief receive isReady command
   * \return true, if workpiece is ready or false, if workpiece is not ready
   */
  bool isReady();

  /*!
   * \fn processQueue()
   * \brief processing the queue
   */
  void processQueue();

private:
  //modbus_t* mb; // connection to mps
  //const char* ip;
  //int port; // port for modbuscommunication
};

#endif // MPSPICKPLACE1_H
