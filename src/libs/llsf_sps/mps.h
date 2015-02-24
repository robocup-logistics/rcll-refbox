/*!
* \file mps.h
* \brief Definition of a mps machine
* \author David Masternak
* \version 1.0
*/

#ifndef MPS_H
#define MPS_H

class MPS {
 public:
  virtual ~MPS() = 0;

  /*!
   * \fn receiveData()
   * \brief receive data from MPS
   */
  virtual void receiveData() = 0;
};

#endif // MPS_H
