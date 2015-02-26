/*!
* \file mps_message.cpp
* \brief Definition of a mps_message
* \author David Masternak
* \version 1.0
*/

#include <mps_message.h>
  
/*!
 * \fn MPSMessage(int from, int to, int prio, int[] msg)
 * \brief Constructor
 * \param from address of sender
 * \param to address of receiver
 * \param prio priority of message
 * \param msg content of message
 */
MPSMessage::MPSMessage(int from, int to, int prio, int[] msg) {
  this->from = from;
  this->to = to;
  this->prio = prio;
  this->msg = msg;
}

/*!
 * \fn setFrom(int from)
 * \brief setter method for sender
 * \param from address of sender
 */
void MPSMessage::setFrom(int from) {
  this->from = from;
}

/*!
 * \fn setTo(int to)
 * \brief setter method for receiver
 * \param to address of receiver
 */
void MPSMessage::setTo(int to) {
  this->to = to;
}

/*!
 * \fn setPrio(int prio)
 * \brief setter method for priotity
 * \param prio priority of message
 */
void MPSMessage::setPrio(int prio) {
  this->prio = prio;
}

/*!
 * \fn setMsg(int[] msg)
 * \brief setter method for msg
 * \param msg content of message
 */
void MPSMessage::setMsg(int[] msg) {
  this->msg = msg;
}
  
/*!
 * \fn getFrom()
 * \brief getter method for sender
 * \return address of sender
 */
int MPSMessage::getFrom() {
  return this->from;
}
  
/*!
 * \fn getTo()
 * \brief getter method for receiver
 * \return address of reveiver
 */
int MPSMessage::getTo() {
  return this->to;
}

/*!
 * \fn getPrio()
 * \brief getter method of priotity
 * \return priority of the message
 */
int MPSMessage::getPrio() {
  return this->prio;
}
  
/*!
 * \fn getMsg()
 * \brief getter method of msg
 * \return array of message
 */
int[] MPSMessage::getMsg() {
  return this->msg;
}
