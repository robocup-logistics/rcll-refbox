/***************************************************************************
 *  mps_refbox_interface.cpp - Thread handling for MPS stations
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
* \file mps_refbox_interface.cpp
* \brief Implementation of communication between Refbox and server
* \author David Masternak
* \version 1.0
*/

#include "mps_refbox_interface.h"

#include "stations.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

/*!
* \fn MPSRefboxInterface()
* \brief Constructor
*/
MPSRefboxInterface::MPSRefboxInterface()
{
	__max_thread_time_usec = 00000; // thread timeout in microsecs
	__max_thread_time_sec  = 1;
	mpsThreadList          = new ThreadList(true, "MPSList");
}

MPSRefboxInterface::MPSRefboxInterface(const char *name)
{
	__max_thread_time_usec = 00000; // thread timeout in microsecs
	__max_thread_time_sec  = 1;
	mpsThreadList          = new ThreadList(true, "MPSList");
}

/*!
* \fn ~MPSRefboxInterface()
* \brief Destructor
*/
MPSRefboxInterface::~MPSRefboxInterface()
{
}

void
MPSRefboxInterface::process()
{
	std::list<std::string> recovered_threads;
	mpsThreadList->try_recover(recovered_threads);

	if (recovered_threads.size() > 0) {
		printf("Recovered threads\n");
		for (std::list<std::string>::iterator i = recovered_threads.begin();
		     i != recovered_threads.end();
		     ++i) {
			printf("  %s\n", i->c_str());
		}
	}

	try {
		std::list<std::string> bad_threads;
		mpsThreadList->wakeup_and_wait(__max_thread_time_sec, __max_thread_time_usec, bad_threads);

		/*
    for (const std::string &badthreadname : bad_threads) {
      for (Thread *t1 : *mpsThreadList) {
	  if (t1->name() == badthreadname) {
          if (dynamic_cast<Machine*>(t1)->machineState == dynamic_cast<MPS*>(t1)->DISCONNECTED) {
            dynamic_cast<Machine*>(t1)->reconnect();
          }
        }
      }
    }
    */
	} catch (fawkes::Exception &e) {
		e.print_trace();
	}
}

void
MPSRefboxInterface::insertMachine(std::string station_name, Machine *mps)
{
	mpses_[station_name] = mps;
	//t->start();
	//mpsThreadList->push_back(mps);
}

std::map<std::string, std::string>
MPSRefboxInterface::get_states()
{
	std::map<std::string, std::string> threadStates;

	//for(Thread *t : *mpsThreadList) {
	//  threadStates[t->name()] = dynamic_cast<MPS*>(t)->machienStateString();
	//}

	return threadStates;
}
