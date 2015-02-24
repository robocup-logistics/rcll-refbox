/*!
* \file IMPSDeliver.h
* \brief Abstract definitions for Deliver communication
* \author David Masternak
* \version 1.0
*/

#ifndef IMPSDELIVER_H
#define IMPSDELIVER_H

#include <MPS.h>

/*!
* \class IMPSDeliver
* \brief Communication between Refbox and Delivery Station
*/
class IMPSDeliver : public MPS {
 public:
  /*!
   * \fn sendDeliver(int lane)
   * \brief send deliver command
   * \param lane on which lane have to be deliver
   */
  virtual void sendDeliver(int lane) = 0;
  
  /*!
   * \fn isDelivered()
   * \brief receive isDelivered data
   * \param ready received data
   * \return true, if delivery was successful and false if not
   */
  virtual bool isDelivered(bool ready) = 0;
};

#endif // IMPSDELIVER_H
