/***************************************************************************
 *  MainWindow.cpp - Central GUI element for the LLSFVIS
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

#include "MainWindow.h"
#include <iostream>

namespace LLSFVis {

MainWindow::MainWindow() {

	set_default_size(1000, 800);
	set_position(Gtk::WIN_POS_CENTER);
	_vPaned = new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL);
	add(*_vPaned);
	_playField = new PlayField();

	Machine* m = new Machine(1.2, 1.0, Machine::NORTH, "machine_1\ntype: T1");
	Puck* p = new Puck(2.5,2.5,"12","raw");

	_playField->add_machine(m);
	_playField->add_puck(p);
	_buttonBox = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
	_button1 = new Gtk::Button("Button1");
	_button2 = new Gtk::Button("Button2");
	_button3 = new Gtk::Button("Button3");
	_button4 = new Gtk::Button("Button4");
	_buttonBox->pack_start(*_button1, Gtk::PACK_SHRINK);
	_buttonBox->pack_start(*_button2, Gtk::PACK_SHRINK);
	_buttonBox->pack_start(*_button3, Gtk::PACK_SHRINK);
	_buttonBox->pack_start(*_button4, Gtk::PACK_SHRINK);

	//_vPaned->set
	_vPaned->add1(*_playField);
	_vPaned->add2(*_buttonBox);

	_button1->show();
	_button2->show();
	_button3->show();
	_button4->show();

	_buttonBox->show();
	_playField->show();
	_vPaned->show();
	show();

}

MainWindow::~MainWindow() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
