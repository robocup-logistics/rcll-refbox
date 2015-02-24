/*!
* \file IMPSIncomingStation.h
* \brief Abstract declaration for IncomingStation communication
* \author David Masternak
* \version 1.0
*/

#ifndef IMPSINCOMINSTATION_H
#define IMPSINCOMINSTATION_H

#include <MPS.h>

/*!
* \class IMPSIncomingStation
* \brief Abstract declaration for IncomingStation communication
*/
class IMPSIncomingStation : public MPS{
 public:
  /*!
   * \fn getCap(int color, int line)
   * \brief send getCap command
   * \param color what kind of color we have to provide
   */
  virtual void getCap(int color, int side) = 0;

  /*!
   * \fn capReady()
   * \brief receive capReady command
   * \param ready received data
   * \return true if cap is ready and false if not
   */
  virtual bool capReady(bool ready) = 0;

  /*!
   * \fn isEmpty(int color)
   * \brief receive isEmpty command
   * \param received empty data
   * \param received empty command for color
   * \return true if lane is empty and false if not
   */
  virtual bool isEmpty(bool empty, int color) = 0;
};

#endif // IMPSINCOMINSTATION_H
