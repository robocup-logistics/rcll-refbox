/*!
* \file mps_message.h
* \brief Definition of a mps_message
* \author David Masternak
* \version 1.0
*/

#ifndef MPSMESSAGE_H
#define MPSMESSAGE_H

/*!
* \class MPSMessage
* \brief Datastruct of a message
*/

class MPSMessage {
 private:
  int from;  // address of sender
  int to;    // address of receiver
  int prio;  // priority of message
  int[] msg; // content of message
  
 public:
  /*!
   * \fn MPSMessage(int from, int to, int prio, int[] msg)
   * \brief Constructor
   * \param from address of sender
   * \param to address of receiver
   * \param prio priority of message
   * \param msg content of message
   */
  MPSMessage(int from, int to, int prio, int[] msg);

  /*!
   * \fn setFrom(int from)
   * \brief setter method for sender
   * \param from address of sender
   */
  void setFrom(int from);

  /*!
   * \fn setTo(int to)
   * \brief setter method for receiver
   * \param to address of receiver
   */
  void setTo(int to);

  /*!
   * \fn setPrio(int prio)
   * \brief setter method for priotity
   * \param prio priority of message
   */
  void setPrio(int prio);

  /*!
   * \fn setMsg(int[] msg)
   * \brief setter method for msg
   * \param msg content of message
   */
  void setMsg(int[] msg);
  
  /*!
   * \fn getFrom()
   * \brief getter method for sender
   * \return address of sender
   */
  int getFrom();
  
  /*!
   * \fn getTo()
   * \brief getter method for receiver
   * \return address of reveiver
   */
  int getTo();

  /*!
   * \fn getPrio()
   * \brief getter method of priotity
   * \return priority of the message
   */
  int getPrio();
  
  /*!
   * \fn getMsg()
   * \brief getter method of msg
   * \return array of message
   */
  int[] getMsg();
};

#endif // MPSMESSAGE_H
