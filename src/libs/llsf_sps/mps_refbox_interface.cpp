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
MPSRefboxInterface::MPSRefboxInterface() : Thread("default") {  
  __mainloop_barrier = new InterruptibleBarrier(13); // 13 Is the count of all MPS stations and the refbox itself
  __max_thread_time_usec = 30000;
  __max_thread_time_nanosec = __max_thread_time_usec * 1000;
}

MPSRefboxInterface::MPSRefboxInterface(const char* name): Thread(name) {

}
/*!
* \fn ~MPSRefboxInterface()
* \brief Destructor
*/
MPSRefboxInterface::~MPSRefboxInterface() {}

void MPSRefboxInterface::loop() {
  
}

void MPSRefboxInterface::once() {}

//void MPSRefboxInterface::set_mainloop_thread(Thread *mainloop_thread) {}

void MPSRefboxInterface::full_start() {}
