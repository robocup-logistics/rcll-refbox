/*!
* \file mps.h
* \brief Definition of a mps machine
* \author David Masternak
* \version 1.0
*/

#ifndef MPS_H
#define MPS_H

#include <queue>
#include <modbus/modbus.h>

/*!
* \class MPS
* \brief Abstract definition of a mps
*/
class MPS {
 private:
  bool lock; // mps is locked because it process something

 public:
  /*!
   * \fn MPS()
   * \brief Constructor
   */
  MPS();

  /*!
   * \fn ~MPS()
   * \brief Destructor
   */
  ~MPS();
  
  /*!
   * \fn isLocked()
   * \brief getter for lock
   * \return true if mps is locked and false if not
   */
  bool isLocked() {
    return this->lock;
  }
  
  /*!
   * \fn setLocked(bool lock)
   * \brief setter for lock
   * \param lock true if mps is locked and false if not
   */
  void setLock(bool lock) {
    this->lock = lock;
  }
};

#endif // MPS_H
