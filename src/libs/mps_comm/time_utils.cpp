// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  time_utils.cpp - Helper functions to handle timespecs
 *
 *  Created: Thu 21 Feb 2019 13:29:11 CET 13:29
 *  Copyright  2019  Alex Maestrini <maestrini@student.tugraz.at>
 *                   Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#include "time_utils.h"

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

void
timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec  = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	} else {
		result->tv_sec  = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

} // namespace mps_comm
} // namespace rcll
