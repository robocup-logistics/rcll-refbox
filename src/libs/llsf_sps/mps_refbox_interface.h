/*!
* \file mps_refbox_interface.h
* \brief Definition of communication between Refbox and Modbus server
* \author David Masternak
* \version 1.0
*/

#ifndef MPSREFBOXINTERFACE_H
#define MPSREFBOXINTERFACE_H

#include <core/threading/thread_list.h>
#include <map>
#include "mps.h"

using namespace fawkes;

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface {
 private:
  unsigned int          __max_thread_time_usec;
  unsigned int          __max_thread_time_sec;
  ThreadList           *mpsThreadList;
  
 public:
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

  void process();

  std::map<std::string, std::string> get_states();

};
#endif // MPSREFBOXINTERFACE_H
