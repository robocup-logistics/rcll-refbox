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

#include <llsf_sps/mps.h>
#include <llsf_sps/mps_incoming_station.h>
#include <llsf_sps/mps_pick_place_1.h>
#include <llsf_sps/mps_pick_place_2.h>
#include <llsf_sps/mps_deliver.h>

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
  std::map<std::string, MPS *>  mpses_;
  
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
  void insertMachine(std::string station_name, MPS *mps, Thread *t);

  void process();

  std::map<std::string, std::string> get_states();

  template<class C>
  C get_station(const std::string &name, C &machine)
  {
    if (mpses_.find(name) == mpses_.end()) {
      return NULL;
    }

    MPS *mps = mpses_[name];
    C typed_mps = dynamic_cast<C>(mps);
    if (! typed_mps) {
      return NULL;
    }
    machine = typed_mps;
    return machine;
  }

};
#endif // MPSREFBOXINTERFACE_H
