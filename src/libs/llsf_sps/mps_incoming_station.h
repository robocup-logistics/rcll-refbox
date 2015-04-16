/*!
* \file mps_deliver.h
* \brief Definitions for Deliver communication
* \author David Masternak
* \version 1.0
*/

#ifndef MPSINCOMINGSTATION_H
#define MPSINCOMINGSTATION_H

#include "mps.h"

/*!
* \class MPSIncomingStation
* \brief Communication between Refbox and Incoming Station
*/
class MPSIncomingStation : public MPS {
 public:
  enum MachineState {IDLE, AVAILABLE, PROCESSING, DELIVERED};
  
  /*!
   * \fn MPSIncomingStation(char* ip, int port)
   * \brief Constructor
   * \param ip address of mps
   * \param port port for modbus communication
   */
  MPSIncomingStation(char* ip, int port);
 
  /*!
   * \fn ~MPSIncomingStation()
   * \brief Destructor
   */
  ~MPSIncomingStation();
  
  /*!
   * \fn getCap(int color, int side)
   * \brief send getCap command
   * \param color color of workpiece that have to be provide
   * \param side on which side workpiece have to be provide
   */
  void getCap(int color, int side);

  /*!
   * \fn capReady()
   * \brief receive capReady command
   * \return true if cap is ready and false if not
   */
  bool capReady();

  /*!
   * \fn isEmpty()
   * \brief receive isEmpty command
   * \return value of empty lanes
   */
  int isEmpty();

  /*!
   * \fn isAvailable()
   * \brief receive isAvailable command
   * \return available
   */
  int isAvailable();
  
  /*!
   * \fn isProcessing()
   * \brief receive isProcessing command
   * \return processing
   */
  int isProcessing();

  /*!
   * \fn isDelivered()
   * \brief receive isDelivered command
   * \return delivered
   */
  int isDelivered();
  
  /*!
   * \fn setLight(int light, int state);
   * \param light what color
   * \param state on or off
   */
  void setLight(int light, int state, int blink);

  /*!
   * \fn clearRegister();
   * \brief set all registeres to 0
   */
  void clearRegister();
  
  /*!
   * \fn processQueue()
   * \brief processing the queue
   */
  //void processQueue();

  /*!
   * \fn getState()
   * \brief get the state of the machine
   * \return machinestate
   */
  MachineState getState();
 private:
  int lastId;
};

#endif // MPSINCOMINGSTATION_H
