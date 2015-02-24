/*!
* \file IMPSPickPlace2.h
* \brief Abstract declaration for Pick&Place 2 communication
* \author David Masternak
* \version 1.0
*/

#ifndef IMPSPICKPLACE2_H
#define IMPSPICKPLACE2_H

#include <MPS.h>

/*!
* \class IMPSPickPlace1
* \brief Abstract declaration for Pick&Place 2 communication
*/
class IMPSPickPlace2 : public MPS{
 public:
  /*!
   * \fn produceRing(int workpiece)
   * \brief send command to produce ring
   * \param workpiece what kind of workpiece have to be assambled
   */
  virtual void produceRing(int workpiece) = 0;

  /*!
   * \fn ringReady()
   * \brief receive ringReady command
   * \param ready received ringReady command
   * \return true if workpiece is ready and false if not
   */  
  virtual bool ringReady(bool ready) = 0;

  /*!
   * \fn isEmpty()
   * \brief receive isEmpty command
   * \param lane for which lane we reveived the data
   * \param empty received isEmpty command
   * \return true if lane is empty and false if not
   */  
  virtual bool isEmpty(int lane, bool empty) = 0;
};

#endif // IMPSPICKPLACE2
