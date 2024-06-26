
// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  qa_socket_datagram_broadcast.cpp - Fawkes QA BroadcastDatagramSocket
 *
 *  Created: Fri 02 Apr 2010 03:15:49 PM CEST
 *  Copyright  2006-2007  Tim Niemueller [www.niemueller.de]
 *  Copyright  2010  Christoph Schwering (copied from Tim's
 *  qa_socket_datagram_multicast.cpp)
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

/// @cond QA

/* NOTE:
 * This program does not do any error correction, if a number is not received
 * by the reflector, this may stall. On wireless networks this is usually
 * the case for an i << 100, often even i < 10. If you use a cable connection
 * this problem does not occur. Meaning that the connection stalls is not an
 * indicator for a broken implementation, as long as you can do this with a
 * reliable connection like a cabled LAN for a long time (stopped my tests
 * at i ~ 1000).
 */

#include <core/threading/thread.h>
#include <netcomm/socket/datagram_broadcast.h>
#include <netinet/in.h>
#include <utils/system/argparser.h>
#include <utils/system/signal.h>

#include <cstdio>
#include <cstring>
#include <netdb.h>

#define BROADCAST_IP "172.16.35.255"

using namespace fawkes;

class BroadcastDatagramServerThread : public Thread
{
public:
	BroadcastDatagramServerThread(unsigned short int port)
	: Thread("BroadcastDatagramServerThread", Thread::OPMODE_CONTINUOUS)
	{
		i = 0;
		try {
			s = new BroadcastDatagramSocket(BROADCAST_IP, port);
			s->bind();
		} catch (Exception &e) {
			e.print_trace();
			throw;
		}
	}

	~BroadcastDatagramServerThread()
	{
		printf("Closing server socket\n");
		s->close();
		printf("Closed server socket\n");
		delete s;
	}

	virtual void
	loop()
	{
		try {
			printf("Sending %u\n", i);
			s->send(&i, sizeof(i));
			printf("Sent %u\n", i);
			unsigned int ri = 0;
			from_len        = sizeof(from);
			printf("Receiving\n");
			s->recv(&ri, sizeof(ri), (struct sockaddr *)&from, &from_len);
			if (ri != i) {
				printf("ERROR: sent %u but received %u\n", i, ri);
			} else {
				printf("OK: sent %u and received %u\n", i, ri);
			}
			++i;
		} catch (Exception &e) {
			printf("Loop failed\n");
			e.print_trace();
			throw;
		}
	}

private:
	unsigned int             i;
	BroadcastDatagramSocket *s;
	struct sockaddr_in       from;
	unsigned int             from_len;
};

class BroadcastDatagramReflectorThread : public Thread
{
public:
	BroadcastDatagramReflectorThread(unsigned short int port)
	: Thread("BroadcastDatagramReflectorThread", Thread::OPMODE_CONTINUOUS)
	{
		try {
			s = new BroadcastDatagramSocket("224.16.0.1", port);
			s->bind();
		} catch (Exception &e) {
			e.print_trace();
			throw;
		}
		from_len = sizeof(from);
	}

	~BroadcastDatagramReflectorThread()
	{
		printf("Closing reflector socket\n");
		s->close();
		printf("Closed reflector socket\n");
		delete s;
	}

	virtual void
	loop()
	{
		unsigned int i = 0;
		printf("Waiting for data to reflect\n");
		s->recv(&i, sizeof(i), (struct sockaddr *)&from, &from_len);
		printf("Received %u, reflecting\n", i);
		s->send(&i, sizeof(i));
	}

private:
	BroadcastDatagramSocket *s;
	struct sockaddr_in       from;
	unsigned int             from_len;
};

class BroadcastDatagramSocketQAMain : public SignalHandler
{
public:
	BroadcastDatagramSocketQAMain(ArgumentParser &argp)
	{
		s = NULL;
		r = NULL;
		if (argp.has_arg("r")) {
			printf("Going to be a reflector\n");
			r = new BroadcastDatagramReflectorThread(1910);
		} else {
			s = new BroadcastDatagramServerThread(1910);
		}
	}

	~BroadcastDatagramSocketQAMain()
	{
		delete s;
		delete r;
	}

	virtual void
	handle_signal(int signum)
	{
		printf("Signal received, cancelling threads\n");
		if (s != NULL)
			s->cancel();
		if (r != NULL)
			r->cancel();
		printf("Threads cancelled\n");
	}

	void
	run()
	{
		if (s != NULL) {
			s->start();
			s->join();
		}
		if (r != NULL) {
			r->start();
			r->join();
		}
	}

private:
	BroadcastDatagramServerThread    *s;
	BroadcastDatagramReflectorThread *r;
};

int
main(int argc, char **argv)
{
	printf("Going to broadcast to " BROADCAST_IP "\n");
	ArgumentParser                argp(argc, argv, "r");
	BroadcastDatagramSocketQAMain m(argp);
	SignalManager::register_handler(SIGINT, &m);
	SignalManager::ignore(SIGPIPE);

	m.run();
	return 0;
}

/// @endcond
