/*!
* \file MPSPickPlace2.h
* \brief Definition of communication between Refbox and Pick&Place 2
* \author David Masternak
* \version 1.0
*/

#ifndef MPSPICKPLACE2_H
#define MPSPICKPLACE2_H

#include <IMPSPickPlace2.h>
#include <MPSRefboxInterface.h>

/*!
* \class MPSPickPlace2
* \brief This class handels the communication to a Pick&Place 2
*/
class MPSPickPlace2 : public IMPSPickPlace2 {
 public:
  /*!
   * \fn MPSPickPlace2(MPSRefboxInterface* cli, int addr)
   * \brief Initialize communication
   * \param cli reference to refbox client
   * \param addr address of destination MPS
   */
  MPSPickPlace2(MPSRefboxInterface* cli, int addr);
  
  /*!
   * \fn produceRing(int workpiece)
   * \brief send command to produce workpiece
   * \param workpiece from which lane
   */
  void produceRing(int workpiece);

  /*!
   * \fn ringReady()
   * \brief receive ringReady command
   * \param ready received ringReady command
   * \return true if workpiece is ready and false if not
   */
  bool ringReady(bool ready);

  /*!
   * \fn isEmpty()
   * \brief receive isEmpty command
   * \param lane for which lane we received the data
   * \param empty reveived isEmpty command
   * \return true if lane is empty and false if not
   */
  bool isEmpty(int lane, bool empty);

  void receiveData();

 private:
  modbus_t* ctx; // connection to server
  int addr; // address of destination MPS
};

#endif // MPSPICKPLACE2_H
