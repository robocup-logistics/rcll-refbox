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

//using namespace Gtk;
int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv,
			"org.gtkmm.examples.base");

	LLSFVis::PlayField pf;

	pf.add_machine(3.92, 1.68, 0, "M1", "T0");
	pf.add_machine(3.92, 3.92, 0, "M2", "T0");
	pf.add_machine(2.80, 0.56, 0, "M3", "T0");
	pf.add_machine(2.80, 1.68, 0, "M4", "T0");
	pf.add_machine(2.80, 2.80, 0, "M5", "T0");
	pf.add_machine(2.80, 3.92, 0, "M6", "T0");
	pf.add_machine(2.80, 5.04, 0, "M7", "T0");
	pf.add_machine(1.68, 1.68, 0, "M8", "T0");
	pf.add_machine(1.68, 3.92, 0, "M9", "T0");
	pf.add_machine(1.68, 5.04, 0, "M10", "T0");

	pf.add_machine(5.40, 0.20, M_PI * 0.25, "R1", "RU");
	pf.add_machine(0.20, 5.40, M_PI * 1.25, "R2", "RU");
	pf.add_machine(5.40, 5.40, M_PI * 0.75, "T", "TU");

	LLSFVis::GameState gs;
	gs.setMachines(pf.getMachines());
	gs.setPhase(LLSFVis::GameState::PRESTART);

	std::list<const LLSFVis::Puck*> pucks;
	LLSFVis::Puck* p = new LLSFVis::Puck(2.3, 2.3, "p1", "S0");
	pucks.push_back(p);

	gs.setPucks(pucks);

	std::list<const LLSFVis::Robot*> bots;
	LLSFVis::Robot* r1 = new LLSFVis::Robot("Tick");
	r1->setPose(2.5, 2.5, 0);
	r1->setLastHeard(time(NULL));
	bots.push_back(r1);
	LLSFVis::Robot* r2 = new LLSFVis::Robot("Trick");
	r2->setPose(3.5, 3.5, M_PI_2);
	r2->setLastHeard(time(NULL) - 3);
	bots.push_back(r2);
	LLSFVis::Robot* r3 = new LLSFVis::Robot("Track");
	r3->setPose(0.5, 2.5, M_PI_2 / 2);
	r3->setLastHeard(time(NULL) - 5);
	bots.push_back(r3);

	gs.setRobots(bots);
	gs.setScore(42);
	gs.setTime(123);

	LLSFVis::MainWindow mainWindow;
	mainWindow.set_playfield(pf);
	mainWindow.add_log_message("Good Morning");
	mainWindow.set_attention_msg("ATTENTION! ROBOT'S GONNA BLOW UP!");
	mainWindow.update_game_state(gs);
	return app->run(mainWindow);

}

