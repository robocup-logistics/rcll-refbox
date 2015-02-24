/*!
* \file IMPSBand.h
* \brief Abstract definitions for communication between Refbox and Band
* \author David Masternak
* \version 1.0
*/

#ifndef IMPSBAND_H
#define IMPSBAND_H

#include <MPS.h>

/*!
* \class IMPSBand
* \brief Communication between Refbox and MPS. What kind of MPS we want use.
*/
class IMPSBand : public MPS {
 public:
  /*!
   * \fn runBand()
   * \brief sending the command to run the band
   */
  virtual void runBand() = 0;

  /*!
   * \fn isReady()
   * \brief receive isReady command
   * \param ready received data
   * \return true if workpiece is ready and false if not
   */
  virtual bool isReady(bool ready) = 0;
};

#endif // IMPSBAND_H
