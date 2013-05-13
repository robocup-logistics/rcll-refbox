/***************************************************************************
 *  llsfvis.cpp - LLSF Visualization Main Program
 *
 *  Created: Mon Jan 31 17:46:35 2013
 *  Copyright  2013  Daniel Ewert (daniel.ewert@ima.rwth-aachen.de)
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

#include <gtkmm.h>
#include "MainWindow.h"
#include "RefboxClient.h"
#include "msgs/MachineInfo.pb.h"
#include "test_data.cpp"
#include <pthread.h>

#define DEBUG true

using namespace llsf_msgs;
using namespace std;



LLSFVis::MainWindow* mWindow_;

void* send_game_states(void* arg) {
	GameState* gs = new GameState();
	gs->set_phase(GameState_Phase_EXPLORATION);
	gs->set_state(GameState_State_RUNNING);
	Time* t = gs->mutable_game_time();
	float seconds =0;
	while (true) {
		seconds+=0.1;
		t->set_sec((int)seconds);
		mWindow_->update_game_state(*gs);
		usleep(100000);
	}
	return 0;
}


int main(int argc, char* argv[]) {
	pthread_t p;


	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv,
			"org.gtkmm.examples.base");

	LLSFVis::MainWindow mainWindow;
	mWindow_ = &mainWindow;
	LLSFVis::RefboxClient refboxClient(mainWindow);
	mainWindow.signal_remove_puck().connect(
			sigc::mem_fun(refboxClient,
					&LLSFVis::RefboxClient::on_signal_send_msg));

	mainWindow.signal_set_game_state().connect(
			sigc::mem_fun(refboxClient,
					&LLSFVis::RefboxClient::on_signal_send_msg));
	if (DEBUG) {
		mainWindow.add_log_message("Good Morning");
	}
	pthread_create(&p, NULL, &send_game_states, NULL);
	return app->run(mainWindow);

}

