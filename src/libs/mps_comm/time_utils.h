/***************************************************************************
 *  time_utils.h - Helper functions to handle timespecs
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

// Copied from https://gist.github.com/diabloneo/9619917
// Author of the cpp file is Neo Liu (diabloneo)
// Code to calculate difference of two timespecs
#pragma once
#include <ctime>

namespace rcll {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif
void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result);

} // namespace mps_comm
} // namespace rcll
