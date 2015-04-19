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

#include <core/threading/thread.h>
#include <core/threading/interruptible_barrier.h>
#include <core/threading/thread_list.h>

#include "mps.h"

using namespace fawkes;

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface : public Thread {
 private:
  unsigned int          __max_thread_time_usec;
  unsigned int          __max_thread_time_sec;
  ThreadList           *mpsThreadList;
  bool                 firstTime;
  
 public:
  std::vector<void*> mpsList;
  
  /*!
   * \fn MPSRefboxInterface()
   * \brief Constructor
   */
  MPSRefboxInterface();

  MPSRefboxInterface(const char* name);

  /*!
   * \fn ~MPSRefboxInterface()
   * \brief Destructor
   */
  ~MPSRefboxInterface();

  /*!
   * \fn insertMachine(Thread *t)
   * \brief insert machine into thread list
   * \param t Reference to thread that have to be insert into list
   */
  void insertMachine(Thread *t);

  void loop();
};
#endif // MPSREFBOXINTERFACE_H
