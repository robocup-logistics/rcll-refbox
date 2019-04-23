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

#include "machine.h"
#include "stations.h"

using namespace fawkes;
using namespace llsfrb::modbus;

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface {
 private:
  unsigned int          __max_thread_time_usec;
  unsigned int          __max_thread_time_sec;
  ThreadList           *mpsThreadList;
  std::map<std::string, Machine *>  mpses_;
  
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
   * \param mps The Machine object to insert
   */
  void insertMachine(std::string station_name, Machine *mps);

  void process();

  std::map<std::string, std::string> get_states();

  template<class C>
  C get_station(const std::string &name, C &machine)
  {
    if (mpses_.find(name) == mpses_.end()) {
      return NULL;
    }

    Machine *mps = mpses_[name];
    C typed_mps = dynamic_cast<C>(mps);
    if (! typed_mps) {
      return NULL;
    }
    machine = typed_mps;
    return machine;
  }

};
#endif // MPSREFBOXINTERFACE_H
