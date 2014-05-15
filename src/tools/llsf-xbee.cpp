
/***************************************************************************
 *  llsf-xbee.cpp - LLSF XBee control tool
 *
 *  Created: Wed Apr 16 12:28:37 2014
 *  Copyright  2014  Tim Niemueller [www.niemueller.de]
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

#include <utils/system/argparser.h>
#include <xbee/xbee.h>

#include <unistd.h>

using namespace llsf_xbee;
using namespace fawkes;

void
usage(const char *progname)
{
  printf("Usage: %s [-q] [-d]\n"
	 "-h               This help message\n"
	 "-d               Enable XBee debugging\n"
	 "-q               Query configuration\n",
	 progname);
}


void
message_received(std::shared_ptr<XBeeRxData> rx_data)
{
  printf("Received RX data: ");
  rx_data->print();
}

int
main(int argc, char **argv)
{
  ArgumentParser argp(argc, argv, "hqd");

  if (argp.has_arg("h")) {
    usage(argv[0]);
    exit(0);
  }

  std::auto_ptr<llsfrb::Configuration> config(new llsfrb::YamlConfiguration(CONFDIR));
  config->load("config.yaml");

  //printf("Connecting to SPS...\n");  

  XBee xbee("/dev/ttyUSB0", 9600);
  xbee.set_debug(argp.has_arg("d"));

  xbee.signal_rcvd_rx_data().connect(&message_received);

  printf("PAN ID: %x\n", xbee.get_pan_id());
  printf("Network Address: %x\n", xbee.get_network_address());
  uint64_t hwaddr = xbee.get_hardware_address();
  printf("Hardware Address: %08lx %08lx\n",
	 hwaddr >> 32, hwaddr & 0xffffffff);

  /*
  xbee.discover();
  sleep(4);
  std::map<uint64_t, XBeeNode> nodes = xbee.get_nodes();
  for (auto n : nodes) {
    printf("Found node %s (%x, %08lx %08lx)\n", n.second.identifier.c_str(),
	   n.second.network_address,
	   n.second.hw_address >> 32, n.second.hw_address & 0xffffffff);

    printf("Sending some garbage\n");
    uint8_t buffer[4];
    buffer[0] = 1;
    buffer[1] = 2;
    buffer[2] = 2;
    buffer[3] = 1;
    xbee.send(n.second.hw_address, n.second.network_address, buffer, 4);
  }
  */

  printf("***\n\n");
  uint64_t hw_addr = 0;
  uint16_t net_addr = 0;
  if (xbee.find_node("M1", hw_addr, net_addr)) {
    printf("Node M1: %08lx %08lx (%04x)\n",
	   hw_addr >> 32, hw_addr & 0xffffffff, net_addr);
  } else {
    printf("Could not find node M1\n");
  }

  printf("Looking for M2\n");
  if (xbee.find_node("M2", hw_addr, net_addr)) {
    printf("Node M2: %08lx %08lx (%04x)\n",
	   hw_addr >> 32, hw_addr & 0xffffffff, net_addr);
  } else {
    printf("Could not find node M1\n");
  }	 

  sleep(8);

  return 0;
}
