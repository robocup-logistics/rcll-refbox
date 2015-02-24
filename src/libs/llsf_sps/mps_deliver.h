/*!
* \file MPSDeliver.h
* \brief Definitions for Deliver communication
* \author David Masternak
* \version 1.0
*/

#ifndef MPSDELIVER_H
#define MPSDELIVER_H

#include <mps_refbox_interface.h>

/*!
* \class MPSDeliver
* \brief Communication between Refbox and Delivery Station
*/
class MPSDeliver {
 public:
  /*!
   * \fn MPSDeliver(MPSRefboxInterface* cli, int addr)
   * \brief Constructor
   * \param cli reference of Refbox Interface
   * \param port port for modbus communication
   */
  MPSDeliver(MPSRefboxInterface* cli, int addr);
  
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
  bool isDelivered(bool ready);

  void receiveData();

 private:
  modbus_t* ctx; // connection to server
  int addr; // address of destination MPS

};

#endif // MPSDELIVER_H
