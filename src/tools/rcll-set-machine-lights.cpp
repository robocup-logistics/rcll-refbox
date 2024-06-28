
// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  rcll-set-lights.cpp - set MPS lights
 *
 *  Created: Mon Mar 21 17:18:21 2016
 *  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#include <msgs/MachineCommands.pb.h>
#include <protobuf_comm/client.h>
#include <utils/system/argparser.h>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

boost::asio::io_service io_service_;

static bool           quit    = false;
ProtobufStreamClient *client_ = NULL;

std::string           machine_name_;
llsf_msgs::LightState light_state_red_;
llsf_msgs::LightState light_state_yellow_;
llsf_msgs::LightState light_state_green_;

void
signal_handler(const boost::system::error_code &error, int signum)
{
	if (!error) {
		quit = true;
		io_service_.stop();
	}
}

void
handle_connected()
{
	printf("Sending State\n");
	llsf_msgs::SetMachineLights set_lights;
	set_lights.set_machine_name(machine_name_);

	llsf_msgs::LightSpec *red = set_lights.add_lights();
	red->set_color(llsf_msgs::RED);
	red->set_state(light_state_red_);

	llsf_msgs::LightSpec *yellow = set_lights.add_lights();
	yellow->set_color(llsf_msgs::YELLOW);
	yellow->set_state(light_state_yellow_);

	llsf_msgs::LightSpec *green = set_lights.add_lights();
	green->set_color(llsf_msgs::GREEN);
	green->set_state(light_state_green_);

	printf("Message:\n"
	       "==================================================================\n"
	       "%s\n"
	       "==================================================================\n",
	       set_lights.DebugString().c_str());

	client_->send(set_lights);

	usleep(200000);
	quit = true;
	io_service_.stop();
}

void
handle_disconnected(const boost::system::error_code &ec)
{
	quit = true;
	io_service_.stop();
}

void
handle_message(uint16_t                                   component_id,
               uint16_t                                   msg_type,
               std::shared_ptr<google::protobuf::Message> msg)
{
}

void
usage(const char *progname)
{
	printf("Usage: %s [-R host[:port]] -m <machine-name> -r <on/off/blink> -y <on/off/blink> -g "
	       "<on/off/blink>\n",
	       progname);
}

int
main(int argc, char **argv)
{
	client_ = new ProtobufStreamClient();

	ArgumentParser argp(argc, argv, "m:r:y:g:R:");

	if (!(argp.has_arg("m") && argp.has_arg("r") && argp.has_arg("y") && argp.has_arg("g"))) {
		usage(argv[0]);
		exit(1);
	}

	if (!argp.has_arg("m")) {
		printf("No machine name given\n");
		exit(-1);
	}
	machine_name_ = argp.arg("m");

	std::string on_off;
	on_off = argp.arg("r");
	if (on_off != "on" && on_off != "off" && on_off != "blink") {
		printf("Invalid red signal state, must be one of (on, off, blink), is %s\n", on_off.c_str());
		exit(-1);
	}
	light_state_red_ =
	  (on_off == "on" ? llsf_msgs::ON : (on_off == "off" ? llsf_msgs::OFF : llsf_msgs::BLINK));

	on_off = argp.arg("y");
	if (on_off != "on" && on_off != "off" && on_off != "blink") {
		printf("Invalid yellow signal state, must be one of (on, off, blink), is %s\n", on_off.c_str());
		exit(-1);
	}
	light_state_yellow_ =
	  (on_off == "on" ? llsf_msgs::ON : (on_off == "off" ? llsf_msgs::OFF : llsf_msgs::BLINK));

	on_off = argp.arg("g");
	if (on_off != "on" && on_off != "off" && on_off != "blink") {
		printf("Invalid green signal state, must be one of (on, off, blink), is %s\n", on_off.c_str());
		exit(-1);
	}
	light_state_green_ =
	  (on_off == "on" ? llsf_msgs::ON : (on_off == "off" ? llsf_msgs::OFF : llsf_msgs::BLINK));

	//MessageRegister & message_register = client_->message_register();

	char              *host      = (char *)"localhost";
	unsigned short int port      = 4444;
	bool               free_host = argp.parse_hostport("R", &host, &port);

	client_->signal_received().connect(handle_message);
	client_->signal_connected().connect(handle_connected);
	client_->signal_disconnected().connect(handle_disconnected);
	client_->async_connect(host, port);

	if (free_host)
		free(host);

#if BOOST_ASIO_VERSION >= 100601
	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(signal_handler);
#endif

	do {
		io_service_.run();
		io_service_.reset();
	} while (!quit);

	delete client_;

	// Delete all global objects allocated by libprotobuf
	google::protobuf::ShutdownProtobufLibrary();
}
