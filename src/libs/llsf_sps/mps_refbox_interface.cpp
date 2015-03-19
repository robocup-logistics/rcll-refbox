/*!
* \file mps_refbox_interface.cpp
* \brief Implementation of communication between Refbox and server
* \author David Masternak
* \version 1.0
*/

#include "mps_refbox_interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>

#include "mps_list.h"

/*!
* \fn MPSRefboxInterface()
* \brief Constructor
*/
MPSRefboxInterface::MPSRefboxInterface() {  
  this->hostList = MPSList::getInstance();
}

/*!
* \fn ~MPSRefboxInterface()
* \brief Destructor
*/
MPSRefboxInterface::~MPSRefboxInterface() {
}

/*!
 * \fn readHostInfo()
 * \brief check for new hosts
 */
void MPSRefboxInterface::readHostInfo() {

}

/*!
 * \fn getHostList()
 * \brief Getter method to get the host list
 * \return vector of all registered hosts
 */
std::vector<MPSInfo*> MPSRefboxInterface::getHostList() {

}
  
/*!
 * \fn setHostList()
 * \brief Setter method to set the host list
 */
void MPSRefboxInterface::setHostList() {

}

/*!
 * \fn insertHost(MPS *mps)
 * \brief Insert a new host into host list
 * \return connection
 */
void MPSRefboxInterface::insertHost(MPSInfo *mps) {

}
  
/*!
 * \fn deleteHost()
 * \brief Delete a host from the host list
 * \return connection
 */
void MPSRefboxInterface::deleteHost() {
  std::cout << std::endl;
}

int main(int argc, char** argv) {

  return 0;
}
