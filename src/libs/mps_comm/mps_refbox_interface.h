/***************************************************************************
 *  mps_refbox_interface.h - Thread handling for MPS stations
 *
 *  Created: Tue 24 Feb 2015 16:03:26 CET 16:03
 *  Copyright  2015  David Masternak <david.masternak@alumni.fh-aachen.de>
 *             2019  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

/*!
* \file mps_refbox_interface.h
* \brief Definition of communication between Refbox and Modbus server
* \author David Masternak
* \version 1.0
*/

#ifndef MPSREFBOXINTERFACE_H
#define MPSREFBOXINTERFACE_H

#include "machine.h"
#include "stations.h"

#include <core/threading/thread_list.h>

#include <map>

using namespace fawkes;
using namespace llsfrb::mps_comm;

/*!
* \class MPSRefboxInterface
* \brief This class handels the communication between refbox and server
*/
class MPSRefboxInterface
{
private:
	unsigned int __max_thread_time_usec;
	unsigned int __max_thread_time_sec;
	ThreadList * mpsThreadList;

public:
	std::map<std::string, Machine *> mpses_;

public:
	/*!
   * \fn MPSRefboxInterface()
   * \brief Constructor
   */
	MPSRefboxInterface();

	MPSRefboxInterface(const char *name);

	/*!
   * \fn ~MPSRefboxInterface()
   * \brief Destructor
   */
	~MPSRefboxInterface();

	/*!
   * \fn insertMachine(Thread *t)
   * \brief insert machine into thread list
   * \param mps The Machine object to insert
   */
	void insertMachine(std::string station_name, Machine *mps);

	void process();

	std::map<std::string, std::string> get_states();

	template <class C>
	C
	get_station(const std::string &name, C &machine)
	{
		if (mpses_.find(name) == mpses_.end()) {
			return NULL;
		}

		Machine *mps       = mpses_[name];
		C        typed_mps = dynamic_cast<C>(mps);
		if (!typed_mps) {
			return NULL;
		}
		machine = typed_mps;
		return machine;
	}
};
#endif // MPSREFBOXINTERFACE_H
