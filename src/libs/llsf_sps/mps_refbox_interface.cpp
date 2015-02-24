/*!
* \file mps_refbox_interface.cpp
* \brief Implementation of communication between Refbox and server
* \author David Masternak
* \version 1.0
*/

#include <mps_refbox_interface.h>
#include <MPSModbusCommunication.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <MPSInfo.h>

/*!
* \fn MPSRefboxInterface(char *cli, int port)
* \brief Constructor
* \param masterip ip of server
* \param port on which port we want communicate with server
*/
MPSRefboxInterface::MPSRefboxInterface(char* masterip, int port, int addr) {
  int rc;
  int nb;
    
  /* TCP */
  ctx = modbus_new_tcp("127.0.0.1", 1502);
  //modbus_set_debug(ctx, TRUE);

  if (modbus_connect(ctx) == -1) {
    std::cout << "ERROR while connecting with master" << std::endl; 
    modbus_free(ctx);
    return;
  }

  /* Allocate and initialize the different memory spaces */
  nb = 1;

  tab_rq_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
  memset(tab_rq_registers, 0, nb * sizeof(uint16_t));

  tab_rp_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
  memset(tab_rp_registers, 0, nb * sizeof(uint16_t));
  
  this->hostList = MPSList::getInstance();

  // for(;;) {
  //   int i;

  //   tab_rq_registers[0] = (uint8_t) 5;

  //   /* SINGLE REGISTER */
  //   std::cout << "Write Data" << std::endl;
  //   rc = modbus_write_register(ctx, 5, tab_rq_registers[0]);
	
  //   if (rc != 1) {
  //     std::cout << "ERROR while writing data to address " << 5 << std::endl;
  //   } 
  //   else {
  //     std::cout << "Read Data" << std::endl;
  //     rc = modbus_read_registers(ctx, 6, 1, tab_rp_registers);
  //     if (rc != 1) {
  // 	std::cout << "ERROR while reading data from address " << 6 << std::endl;
  //     } 
  //     else {
  // 	if (tab_rq_registers[0] != tab_rp_registers[0]) {
  // 	  std::cout << "ERROR while reading data form address " << 5 << ". Input register and output register are not equal" << std::endl;
  // 	  std::cout << "Erwarte: " << tab_rq_registers[0] << std::endl;
  // 	  std::cout << "Received: " << tab_rp_registers[0] << std::endl;
  // 	}
  // 	else {
  // 	  std::cout << "Received data: " << tab_rp_registers[0] << std::endl; 
  // 	}
  //     }
  //   }
  // }

  uint16_t send[2] = {(uint16_t)9, (uint16_t)addr};
  rc = modbus_write_registers(ctx, 1, 2, send);
}

/*!
* \fn ~MPSRefboxInterface()
* \brief Destructor
*/
MPSRefboxInterface::~MPSRefboxInterface() {
    free(tab_rq_registers);
    free(tab_rp_registers);

    modbus_close(ctx);
    modbus_free(ctx);
}


/*!
* \fn getTxpConnection()
* \brief Getter to get the connection
* \return the modbus connection
*/
modbus_t* MPSRefboxInterface::getTcpConnection() {
  return this->ctx;
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

/*!
 * \fn update()
 * \brief do something if new data are available
 * \param the socket/file descriptor where the datas are
 */
void MPSRefboxInterface::update(int descriptor) {
  std::cout << "here the descriptor: " << descriptor << std::endl;
  //std::cout << hosts.size() << std::endl;
  for(int i = 0; i < this->hostList->hosts.size(); i++) {
    if(hostList->hosts.at(i)->getSocket() == descriptor) {
      hostList->hosts.at(i)->getMachine()->receiveData();
    }
  }
}
