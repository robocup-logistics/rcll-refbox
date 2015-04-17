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

#include "mps.h"

using namespace fawkes;

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface : public Thread {
 private:
  InterruptibleBarrier *__mainloop_barrier;
  unsigned int          __max_thread_time_usec;
  unsigned int          __max_thread_time_nanosec;
  unsigned int          __desired_time_usec;
  unsigned int          __desired_time_sec;
  
 public:
  std::vector<void*> mpsList;
  std::vector<Thread*> mpsThreadList;
  
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

  void once();
  void loop();

  //void set_mainloop_thread(Thread *mainloop_thread);

  void full_start();
};
#endif // MPSREFBOXINTERFACE_H
