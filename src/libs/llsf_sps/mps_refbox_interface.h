/*!
* \file mps_refbox_interface.h
* \brief Definition of communication between Refbox and Modbus server
* \author David Masternak
* \version 1.0
*/

#ifndef MPSREFBOXINTERFACE_H
#define MPSREFBOXINTERFACE_H

#include <modbus/modbus.h>
#include <vector>

#include "mps.h"

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface {
 private:
  std::vector<MPS*> mpsList;

 public:
  /*!
   * \fn MPSRefboxInterface()
   * \brief Constructor
   */
  MPSRefboxInterface();

  /*!
   * \fn ~MPSRefboxInterface()
   * \brief Destructor
   */
  ~MPSRefboxInterface();
};

#endif // MPSREFBOXINTERFACE_H
