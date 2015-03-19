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
#include "mps_info.h"
#include "mps_list.h"

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface {
 private:
  MPSList* hostList;

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

  /*!
   * \fn readHostInfo()
   * \brief check for new hosts
   */
  void readHostInfo();

  /*!
   * \fn getHostList()
   * \brief Getter method to get the host list
   * \return vector of all registered hosts
   */
  std::vector<MPSInfo*> getHostList();
  
  /*!
   * \fn setHostList()
   * \brief Setter method to set the host list
   */
  void setHostList();

  /*!
   * \fn insertHost(MPSInfo *mps)
   * \brief Insert a new host into host list
   * \return connection
   */
  void insertHost(MPSInfo *mps);
  
  /*!
   * \fn deleteHost()
   * \brief Delete a host from the host list
   * \return connection
   */
  void deleteHost();
};

#endif // MPSREFBOXINTERFACE_H
