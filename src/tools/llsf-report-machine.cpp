
/***************************************************************************
 *  llsf-report-machine.cpp - report a machine finding
 *
 *  Created: Mon Mar 04 14:09:00 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
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

#include <config/yaml.h>

#include <protobuf_comm/peer.h>
#include <utils/system/argparser.h>

#include <msgs/BeaconSignal.pb.h>
#include <msgs/GameState.pb.h>
#include <msgs/MachineReport.pb.h>


#include <boost/asio.hpp>
#include <boost/date_time.hpp>

using namespace protobuf_comm;
using namespace llsf_msgs;
using namespace fawkes;

static bool quit = false;
std::string machine_name_;
std::string machine_type_;
Team        team_;
ProtobufBroadcastPeer *peer_ = NULL;

void
signal_handler(const boost::system::error_code& error, int signum)
{
  if (!error) {
    quit = true;
  }
}

void
handle_recv_error(boost::asio::ip::udp::endpoint &endpoint, std::string msg)
{
  //printf("Receive error from %s:%u: %s\n",
  //	 endpoint.address().to_string().c_str(), endpoint.port(), msg.c_str());
}

void
handle_send_error(std::string msg)
{
  printf("Send error: %s\n", msg.c_str());
}

void
handle_message(boost::asio::ip::udp::endpoint &sender,
	       uint16_t component_id, uint16_t msg_type,
	       std::shared_ptr<google::protobuf::Message> msg)
{
  std::shared_ptr<BeaconSignal> b;
  if ((b = std::dynamic_pointer_cast<BeaconSignal>(msg))) {
    if (b->team_name() == "LLSF" && b->peer_name() == "RefBox") {
      printf("Announcing machine type\n");
      llsf_msgs::MachineReport report;
      report.set_team_color(team_);
      llsf_msgs::MachineReportEntry *entry = report.add_machines();
      entry->set_name(machine_name_);
      entry->set_type(machine_type_);
      peer_->send(report);
    }
  }

  std::shared_ptr<MachineReportInfo> mrinfo;
  if ((mrinfo = std::dynamic_pointer_cast<MachineReportInfo>(msg))) {
    if (mrinfo->team_color() == team_) {
      printf("Reported machines (%s):", llsf_msgs::Team_Name(team_).c_str());
      for (int i = 0; i < mrinfo->reported_machines_size(); ++i) {
	printf(" %s", mrinfo->reported_machines(i).c_str());
      }
      printf("\n");
    }
  }
}

int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "T:");

  if (argp.num_items() != 2) {
    printf("Usage: %s [-T team] <machine-name> <machine-type>\n"
	   "\n"
	   "-T team	Select team to send for, CYAN (default) or MAGENTA\n",
	   argv[0]);
    exit(1);
  }

  machine_name_ = argp.items()[0];
  machine_type_ = argp.items()[1];

  llsfrb::Configuration *config = new llsfrb::YamlConfiguration(CONFDIR);
  config->load("config.yaml");

  if (config->exists("/llsfrb/comm/peer-send-port") &&
      config->exists("/llsfrb/comm/peer-recv-port") )
  {
    peer_ = new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
				      config->get_uint("/llsfrb/comm/peer-recv-port"),
				      config->get_uint("/llsfrb/comm/peer-send-port"));
  } else {
    peer_ = new ProtobufBroadcastPeer(config->get_string("/llsfrb/comm/peer-host"),
				      config->get_uint("/llsfrb/comm/peer-port"));
  }

  team_ = CYAN;
  if (argp.has_arg("T")) {
    std::string team_str = argp.arg("T");
    if (team_str == "cyan") {
      team_ = CYAN;
    } else if (team_str == "magenta") {
      team_ = MAGENTA;
    } else {
      printf("Unknonw team value, using cyan\n");
    }
  }

  boost::asio::io_service io_service;

  MessageRegister & message_register = peer_->message_register();
  message_register.add_message_type<BeaconSignal>();
  message_register.add_message_type<GameState>();
  message_register.add_message_type<MachineReport>();
  message_register.add_message_type<MachineReportInfo>();

  printf("Waiting for beacon from refbox...\n");

  peer_->signal_received().connect(handle_message);
  peer_->signal_recv_error().connect(handle_recv_error);
  peer_->signal_send_error().connect(handle_send_error);

#if BOOST_ASIO_VERSION >= 100601
  // Construct a signal set registered for process termination.
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

  // Start an asynchronous wait for one of the signals to occur.
  signals.async_wait(signal_handler);
#endif

  do {
    io_service.run();
    io_service.reset();
  } while (! quit);

  delete peer_;
  delete config;

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();
}
