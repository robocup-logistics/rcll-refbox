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

#include "mps_incoming_station.h"
#include "mps_pick_place_1.h"
#include "mps_pick_place_2.h"
#include "mps_deliver.h"

#include "mps_deliver_message.h"

/*!
* \fn MPSRefboxInterface()
* \brief Constructor
*/
MPSRefboxInterface::MPSRefboxInterface() {  
  // Here we have to read a yaml file to get all mps stations
  //MPSDeliver* de1 = new MPSDeliver("192.168.2.20", 1502);
  // MPSPickPlace1* pp1 = new MPSPickPlace1("192.168.2.21", 502);
  // MPSPickPlace2* pp2 = new MPSPickPlace2("192.168.2.22", 502);
  // MPSIncomingStation* is1 = new MPSIncomingStation("192.168.2.21", 502);
  //MPSDeliverSideMessage* msg = new MPSDeliverSideMessage(1);
  //de1->setMessage(msg);
  
  //de1->processQueue();
  //delete de1;

  // MPSPickPlace1 *pp1 = new MPSPickPlace1("127.0.0.1", 1502);
  // std::cout << "Here we are without segfault" << std::endl;
  // pp1->produceEnd(1);
  //this->mpsList.push_back(de1);
}

/*!
* \fn ~MPSRefboxInterface()
* \brief Destructor
*/
MPSRefboxInterface::~MPSRefboxInterface() {}
