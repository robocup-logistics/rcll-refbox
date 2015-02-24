/*!
* \file MPSDeliver.h
* \brief Definitions for Deliver communication
* \author David Masternak
* \version 1.0
*/

#ifndef MPSINCOMINGSTATION_H
#define MPSINCOMINGSTATION_H

#include <mps_refbox_interface.h>

/*!
* \class MPSIncomingStation
* \brief Communication between Refbox and Incoming Station
*/
class MPSIncomingStation {
 public:
  /*!
   * \fn MPSIncomingStation(MPSRefboxInterface* cli, int addr)
   * \brief Constructor
   * \param cli reference of Refbox Interface
   * \param port port for modbus communication
   */
  MPSIncomingStation(MPSRefboxInterface* cli, int addr);
 
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
   * \param ready recived data
   * \return true if cap is ready and false if not
   */
  bool capReady(bool ready);

  /*!
   * \fn isEmpty()
   * \brief receive isEmpty command
   * \param empty received empty command
   * \param color received empty of which color
   * \param color for which color we want have this information
   */
  bool isEmpty(bool empty, int color);

  void receiveData();

 private:
  modbus_t* ctx;
  int addr;
};

#endif // MPSINCOMINGSTATION_H
