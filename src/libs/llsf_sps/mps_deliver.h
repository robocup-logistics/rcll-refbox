/*!
* \file mps_delivery.h
* \brief Definitions for Deliver communication
* \author David Masternak
* \version 1.0
*/

#ifndef MPSDELIVER_H
#define MPSDELIVER_H

#include "mps.h"

#include <core/threading/thread.h>

using namespace fawkes;

/*!
* \class MPSDeliver
* \brief Communication between Refbox and Delivery Station
*/
class MPSDeliver : public MPS, public Thread {
 public:
  enum MachineState {IDLE, AVAILABLE, DELIVER, DELIVERED};
  MachineState machineState;
  
  /*!
   * \fn MPSDeliver(char* ip, int port)
   * \brief Constructor
   * \param ip address of mps
   * \param port port for modbus communication
   */
  MPSDeliver(const char* ip, int port);

  MPSDeliver(const char* ip, int port, const char* name);

  /*!
   * \fn ~MPSDeliver()
   * \brief Destructor
   */
  //~MPSDeliver();
  
  /*!
   * \fn sendDeliver(int lane)
   * \brief send deliver command
   * \param lane on which lane have to be deliver
   */
  void sendDeliver(int lane);

  /*!
   * \fn isDelivered()
   * \brief receive isDelivered data
   * \param ready received data
   * \return true, if delivery was successful and false if not
   */
  int isDelivered();

  /*!
   * \fn setLight(int light, int state);
   * \param light what color
   * \param state on or off
   */
  void setLight(int light, int state, int blink);
  
  /*!
   * \fn processQueue()
   * \brief processing the queue
   */
  //void processQueue();

  /*!
   * \fn clearRegister();
   * \brief set all registeres to 0
   */
  void clearRegister();
  
  MachineState getState();

  void loop();
};

#endif // MPSDELIVER_H
