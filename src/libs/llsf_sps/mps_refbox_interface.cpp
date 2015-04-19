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
  __max_thread_time_usec = 30000; // thread timeout in usecs
  __max_thread_time_sec  = 1;     // thread timeout in nanosecs
  mpsThreadList = new ThreadList("MPSList");
  mpsThreadList->push_back(this);
  firstTime = true;
}

MPSRefboxInterface::MPSRefboxInterface(const char* name): Thread(name) {
  __max_thread_time_usec = 30000; // thread timeout in usecs
  __max_thread_time_sec  = 1;     // thread timeout in nanosecs
  mpsThreadList = new ThreadList("MPSList");
  mpsThreadList->push_back(this);
  firstTime = true;
}

/*!
* \fn ~MPSRefboxInterface()
* \brief Destructor
*/
MPSRefboxInterface::~MPSRefboxInterface() {}

void MPSRefboxInterface::loop() {
  std::list<std::string> recovered_threads;
  mpsThreadList->try_recover(recovered_threads);

  try {
    std::list<std::string> bad_threads;
    mpsThreadList->wakeup_and_wait(__max_thread_time_sec, __max_thread_time_usec, bad_threads);

    for(std::string badthreadname : bad_threads) {
      for(Thread *t1 : *mpsThreadList) {
        if(t1->name() == badthreadname) {
          if(dynamic_cast<MPS*>(t1)->state == dynamic_cast<MPS*>(t1)->DISCONNECTED) {
            dynamic_cast<MPS*>(t1)->reconnect();
          }
        }
      }
    }
  } catch(fawkes::Exception e) {
    
  }
}

void MPSRefboxInterface::insertMachine(Thread* t) {
  mpsThreadList->push_back(t);
}

