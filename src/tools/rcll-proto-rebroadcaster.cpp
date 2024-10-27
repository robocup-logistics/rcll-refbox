// Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

/***************************************************************************
 *  rcll-proto-rebroadcaster.cpp rebroadcast protobuf messages to other ports
 *
 *  Created: 27. Oct 2024
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
#include <protobuf_rebroadcaster/rebroadcaster.h>
#include <utils/system/argparser.h>

#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

using namespace protobuf_comm;
using namespace fawkes;

std::string cfg_file_;

std::condition_variable cv;
std::mutex              cv_m;
bool                    keep_running = true;

void
signal_handler(int signum)
{
	printf("Received signal %d, stopping...\n", signum);
	{
		std::lock_guard<std::mutex> lk(cv_m);
		keep_running = false;
	}
	cv.notify_all();
}

int
main(int argc, char **argv)
{
	// Register the signal handler for SIGINT and SIGTERM
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);

	ArgumentParser argp(argc, argv, "T:");

	std::string cfg_file_;
	if (argp.num_items() > 0) {
		cfg_file_ = argp.items()[0];
	} else {
		cfg_file_ = "config_generated.yaml";
	}

	std::shared_ptr<rcll::YamlConfiguration> config_ =
	  std::make_shared<rcll::YamlConfiguration>(CONFDIR);
	try {
		config_->load(cfg_file_.c_str());
	} catch (...) {
		printf("Error loading %s, trying comm/default_comm.yaml\n", cfg_file_.c_str());
		config_->load("comm/default_comm.yaml");
	}

	// Create ProtoRebroadcaster
	rcll::ProtoRebroadcaster broadcaster(config_);

	// Wait indefinitely until signal is received
	std::unique_lock<std::mutex> lk(cv_m);
	cv.wait(lk, [] { return !keep_running; });

	printf("Exiting gracefully...\n");
	return 0;
}
