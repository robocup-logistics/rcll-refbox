/*!
* \file IMPSPickPlace1.h
* \brief Abstract declaration for Pick&Place 1 communication
* \author David Masternak
* \version 1.0
*/

#ifndef IMPSPICKPLACE1_H
#define IMPSPICKPLACE1_H

#include <MPS.h>

/*!
* \class IMPSPickPlace1
* \brief Abstract declaration for Pick&Place 1 communication
*/
class IMPSPickPlace1 : public MPS {
 public:
  /*!
   * \fn produceEnd()
   * \brief send command to produce workpiece
   */
  virtual void produceEnd() = 0;

  /*!
   * \fn isEmpty()
   * \brief receive isEmpty command
   * \param empty received data
   * \return true, if lane is empty or false, if lane is not empty
   */
  virtual bool isEmpty(bool empty) = 0;

  /*!
   * \fn isReady()
   * \brief receive isReady command
   * \param reveived data
   * \return true, if workpiece is ready or false, if workpiece is not ready
   */
  virtual bool isReady(bool ready) = 0;
};

#endif // IMPSPICKPLACE_H
