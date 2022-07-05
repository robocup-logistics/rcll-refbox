
/***************************************************************************
 *  wait.cpp - TimeWait tool
 *
 *  Created: Thu Nov 29 17:30:37 2007
 *  Copyright  2007  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <utils/time/clock.h>
#include <utils/time/time.h>
#include <utils/time/wait.h>

#include <cstdlib>
#include <unistd.h>

namespace fawkes {

/** @class TimeWait <utils/time/wait.h>
 * Time wait utility.
 * This class allows for guaranteed waiting for a specified amout of time. It can
 * either be used to suspend the current thread for at least the given time (static
 * methods) or it can be used to reach a desired minimum loop time. For this instantiate
 * the class and call mark_start() at the beginning of the loop and wait() at the end.
 * wait() will then suspend the thread as long as needed to have the desired minimum
 * loop time. The TimeWait utility will use the current clock time. Thus it may wait
 * for a given amount of say simulated time.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param clock clock to use
 * @param desired_loop_time_usec desired minimum loop time in microseconds
 */
TimeWait::TimeWait(Clock *clock, long int desired_loop_time_usec)
{
	desired_loop_time_ = desired_loop_time_usec;
	clock_             = clock;
	until_             = new Time();
	until_systime_     = new Time();
	now_               = new Time();
}

/** Destructor. */
TimeWait::~TimeWait()
{
	delete until_;
	delete until_systime_;
	delete now_;
}

/** Mark start of loop. */
void
TimeWait::mark_start()
{
	clock_->get_time(until_);
	*until_ += desired_loop_time_;
	clock_->get_systime(until_systime_);
	*until_systime_ += desired_loop_time_;
}

/** Wait until minimum loop time has been reached. */
void
TimeWait::wait()
{
	clock_->get_time(now_);
	// we want to release run status at least shortly
	usleep(0);

	long int remaining_usec = (*until_ - *now_).in_usec();
	while (remaining_usec > 0) {
		usleep(remaining_usec);
		clock_->get_time(now_);
		remaining_usec = (*until_ - *now_).in_usec();
	}
}

/** Wait until minimum loop time has been reached in real time.
 * This uses the system time and not an external time source if defined.
 */
void
TimeWait::wait_systime()
{
	clock_->get_systime(now_);
	// we want to release run status at least shortly
	usleep(0);

	long int remaining_usec = (*until_systime_ - *now_).in_usec();
	while (remaining_usec > 0) {
		usleep(remaining_usec);
		clock_->get_systime(now_);
		remaining_usec = (*until_systime_ - *now_).in_usec();
	}
}

/** Wait at least usec microseconds.
 * Think of this as an uninterruptible usleep(). This method will not return before
 * *at least* usec microseconds have passed. It may be longer but never less.
 * Time is tracked in system time scale (real time).
 * @param usec number of microseconds to wait at least
 */
void
TimeWait::wait_systime(long int usec)
{
	if (usec < 0)
		return;
	struct timeval start, now;
	long int       remaining_usec = usec;
	gettimeofday(&start, NULL);
	do {
		usleep(remaining_usec);
		gettimeofday(&now, NULL);
	} while ((remaining_usec = usec - time_diff_usec(now, start)) > 0);
}

/** Wait at least usec microseconds.
 * Think of this as an uninterruptible usleep(). This method will not return before
 * *at least* usec microseconds have passed. It may be longer but never less.
 * Time is tracked in the current Clock time scale. This may be simulated time
 * or real time. It is assumed that the (simulated time) is at worst slower, but never
 * faster than real time. Thus 1 microsecond real time is at least 1 microsecond clock time.
 * @param usec number of microseconds to wait at least
 */
void
TimeWait::wait(long int usec)
{
	if (usec < 0)
		return;
	Clock         *clock = Clock::instance();
	struct timeval start, now;
	long int       remaining_usec = usec;
	clock->get_time(&start);
	do {
		usleep(remaining_usec);
		clock->get_time(&now);
	} while ((remaining_usec = usec - time_diff_usec(now, start)) > 0);
}

} // end namespace fawkes
