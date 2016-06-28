
/***************************************************************************
 *  rcll-workpiece.cpp - workpiece handling
 *
 *  Created: Tue Jun 28 17:37:54 2016 (RoboCup 2016, Leipzig)
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

#include <protobuf_comm/client.h>
#include <utils/system/argparser.h>

#include <msgs/BeaconSignal.pb.h>
#include <msgs/OrderInfo.pb.h>
#include <msgs/GameState.pb.h>
#include <msgs/VersionInfo.pb.h>
#include <msgs/ExplorationInfo.pb.h>
#include <msgs/MachineInfo.pb.h>
#include <msgs/MachineReport.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/RingInfo.pb.h>
#include <msgs/WorkpieceInfo.pb.h>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

boost::asio::io_service io_service_;

static bool quit = false;
ProtobufStreamClient *client_ = NULL;


enum {
	SEND_WORKPIECE,
	LIST_WORKPIECES
} opmode_;

std::string  machine_name_;
unsigned int workpiece_id_;
bool         workpiece_visible_;


void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
  }
}

void
handle_connected()
{
	if (opmode_ == SEND_WORKPIECE) {
		printf("Sending workpiece update\n");
		llsf_msgs::Workpiece wp;
		wp.set_id(workpiece_id_);
		wp.set_at_machine(machine_name_);
		printf("Setting vis %s\n", workpiece_visible_ ? "YES" : "NO");
		wp.set_visible(workpiece_visible_);
		client_->send(wp);

		usleep(200000);
		quit = true;
		io_service_.stop();
	}
}

void
handle_disconnected(const boost::system::error_code &ec)
{
  quit = true;
}

void
handle_message(uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
	if (opmode_ == LIST_WORKPIECES) {
		std::shared_ptr<WorkpieceInfo> wi;
		if ((wi = std::dynamic_pointer_cast<WorkpieceInfo>(msg))) {
			if (wi->workpieces_size() == 0) {
				printf("No workpieces currently registered.\n");
			} else {
				for (int i = 0; i < wi->workpieces_size(); ++i) {
					const llsf_msgs::Workpiece &w = wi->workpieces(i);
					printf("WP %3i %8s %8s  (%s)\n",
					       w.id(), w.at_machine().c_str(), llsf_msgs::Team_Name(w.team_color()).c_str(),
					       w.visible() ? "visible" : "not visible");
				}
			}
			quit = true;
			io_service_.stop();
		}
	}
}



void
usage(const char *progname)
{
	printf("Usage: %s list | send <ID> <machine> <visible>\n"
	       "list   List known workpieces and their statuses\n"
	       "send   Send a workpiece update, arguments are:\n"
	       "       <ID> numeric ID\n"
	       "       <machine> Machine at which the workpiece should be reported\n"
	       "       <visible> true or false depending on intended visibility\n",
	       progname);
}


int
main(int argc, char **argv)
{
  client_ = new ProtobufStreamClient();

  ArgumentParser argp(argc, argv, "");

  if (argp.num_items() < 1) {
    usage(argv[0]);
    exit(1);
  }

  std::string command = argp.items()[0];
  if (command == "list") {
	  opmode_ = LIST_WORKPIECES;
  } else if (command == "send") {
	  if (argp.num_items() != 4) {
		  printf("Invalid number of arguments for sending.\n");
		  usage(argv[0]);
		  exit(1);
	  }
	  opmode_ = SEND_WORKPIECE;
	  workpiece_id_ = argp.parse_item_int(1);
	  machine_name_ = argp.items()[2];
	  workpiece_visible_ = (strcmp(argp.items()[3], "true") == 0);
  } else {
	  printf("Unknown command %s\n\n", command.c_str());
	  usage(argv[0]);
    exit(1);
  }

  MessageRegister & message_register = client_->message_register();
  message_register.add_message_type<BeaconSignal>();
  message_register.add_message_type<OrderInfo>();
  message_register.add_message_type<GameState>();
  message_register.add_message_type<VersionInfo>();
  message_register.add_message_type<ExplorationInfo>();
  message_register.add_message_type<MachineInfo>();
  message_register.add_message_type<MachineReportInfo>();
  message_register.add_message_type<RobotInfo>();
  message_register.add_message_type<RingInfo>();
  message_register.add_message_type<WorkpieceInfo>();

  client_->signal_received().connect(handle_message);
  client_->signal_connected().connect(handle_connected);
  client_->signal_disconnected().connect(handle_disconnected);
  client_->async_connect("localhost", 4444);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  do {
    io_service_.run();
    io_service_.reset();
  } while (! quit);

  delete client_;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
