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
#include "msgs/MachineSpec.pb.h"

using namespace llsf_msgs;
int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv,
			"org.gtkmm.examples.base");

	/*
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
	 */
	MachineSpecs mSpecs;
	MachineSpec* m1 = mSpecs.add_machines();
	m1->set_name("M1");
	m1->set_type("T0");
	Pose2D* pose = m1->mutable_pose();
	pose->set_x(3.92);
	pose->set_y(1.68);
	pose->set_ori(0);
	LightSpec* red = m1->add_lights();
	red->set_color(LightColor::RED);
	red->set_state(LightState::ON);
	LightSpec* yellow = m1->add_lights();
	yellow->set_color(LightColor::YELLOW);
	yellow->set_state(LightState::ON);
	LightSpec* green = m1->add_lights();
	green->set_color(LightColor::GREEN);
	green->set_state(LightState::BLINK);

	std::list<const LLSFVis::Puck*> pucks;
	LLSFVis::Puck* p = new LLSFVis::Puck(2.3, 2.3, "p1", "S0");
	pucks.push_back(p);

	RobotInfo rInfo;
	Robot* r = rInfo.add_robots();
	r->set_name("Tick");
	r->set_team("Carologistics");
	Time* rtime = r->mutable_last_seen();
	rtime->set_sec(3);
	rtime->set_nsec(14);
	Pose2D* rpose = r->mutable_pose();
	rpose->set_x(2.8);
	rpose->set_y(2.8);
	rpose->set_ori(0);

	LLSFVis::MainWindow mainWindow;
	//LLSFVis::RefboxClient refboxClient(mainWindow);
	//mainWindow.set_playfield(pf);
	mainWindow.add_log_message("Good Morning");
	mainWindow.update_machines(mSpecs);
	mainWindow.update_robots(rInfo);
	AttentionMessage msg;
	msg.set_message("BEWARE, WE ARE GONNA BLOW UP!!");
	msg.set_time_to_show(10);
	mainWindow.set_attention_msg(msg);
	return app->run(mainWindow);

}

