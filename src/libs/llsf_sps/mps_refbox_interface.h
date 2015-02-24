/*!
* \file MPSRefboxInterface.h
* \brief Definition of communication between Refbox and Modbus server
* \author David Masternak
* \version 1.0
*/

#ifndef MPSREFBOXINTERFACE_H
#define MPSREFBOXINTERFACE_H

#include <iostream>
#include <modbus/modbus.h>
#include <vector>
#include <mps_info.h>
#include <mps_list.h>
#include <observer.h>

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface {
 private:
  uint16_t *tab_rp_registers;
  uint16_t *tab_rq_registers;
  modbus_t* ctx;  
  MPSList* hostList;

 public:
  /*!
   * \fn MPSRefboxInterface(char* masterip, int port)
   * \brief Constructor
   * \param masterip ip of server
   * \param port which port we use for communication
   * \param addr address of Refboxinterface. Default 1
   */
  MPSRefboxInterface(char* masterip, int port, int addr = 1);

  /*!
   * \fn ~MPSRefboxInterface()
   * \brief Destructor
   */
  ~MPSRefboxInterface();

  /*!
   * \fn getTcpConnection()
   * \brief Getter method to get the connection
   * \return connection
   */
  modbus_t* getTcpConnection();

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

  /*!
   * \fn update()
   * \brief do something if new data are available
   * \param socket/file descriptor where the datas are
   */
  void update(int desctiptor);
};

#endif // MPSREFBOXINTERFACE_H
