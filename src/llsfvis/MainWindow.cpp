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

MainWindow::MainWindow() :
		//_playFieldTabGrid(Gtk::ORIENTATION_HORIZONTAL),
		loggingTabPaned_(Gtk::ORIENTATION_HORIZONTAL), buttonBoxPlayField_(
				Gtk::ORIENTATION_VERTICAL), buttonBoxLogging_(
				Gtk::ORIENTATION_VERTICAL) {

	//INITIALISE DATA
	logBuffer_ = Gtk::TextBuffer::create();
	Machine* m = new Machine(1.2, 1.0, Machine::NORTH, "machine_1\ntype: T1");
	Puck* p = new Puck(2.5, 2.5, "12", "raw");
	Robot* r1 = new Robot("Tick");
	r1->setPose(1.0, 2.0, 0);
	Robot* r2 = new Robot("Trick");
	r2->setPose(3.0, 3.0, M_PI*0.2);
	Robot* r3 = new Robot("Track");
	r3->setPose(4.0, 5.0, M_PI * 0.5);

	//SETUP the GUI

	set_default_size(750, 750);
	set_position(Gtk::WIN_POS_CENTER);
	tabs_.set_border_width(10);

	//the PlayField tab

	playField_.add_machine(m);
	playField_.add_puck(p);
	playField_.add_robot(r1);
	playField_.add_robot(r2);
	playField_.add_robot(r3);

	playFieldButton1_.set_label("Button1");
	playFieldButton2_.set_label("Button2");
	playFieldButton3_.set_label("Button3");
	playFieldButton4_.set_label("Button4");

	buttonBoxPlayField_.pack_start(playFieldButton1_, Gtk::PACK_SHRINK);
	buttonBoxPlayField_.pack_start(playFieldButton2_, Gtk::PACK_SHRINK);
	buttonBoxPlayField_.pack_start(playFieldButton3_, Gtk::PACK_SHRINK);
	buttonBoxPlayField_.pack_start(playFieldButton4_, Gtk::PACK_SHRINK);

	logPreviewScrollWindow_.add(logPreview_);
	logPreview_.set_buffer(logBuffer_);

	playFieldTabGrid_.set_row_spacing(5);
	playFieldTabGrid_.set_column_spacing(5);
	playFieldTabGrid_.add(playField_);
	playFieldTabGrid_.attach_next_to(buttonBoxPlayField_, playField_,
			Gtk::POS_RIGHT, 1, 1);
	playFieldTabGrid_.attach_next_to(logPreviewScrollWindow_, playField_,
			Gtk::POS_BOTTOM, 1, 1);
	playFieldTabGrid_.attach_next_to(stateWidget_, buttonBoxPlayField_,
			Gtk::POS_BOTTOM, 1, 1);

	//Create the logging tab

	logButton1_.set_label("log1");
	logButton2_.set_label("log2");
	logButton3_.set_label("log3");
	logButton4_.set_label("log4");

	buttonBoxLogging_.pack_start(logButton1_, Gtk::PACK_SHRINK);
	buttonBoxLogging_.pack_start(logButton2_, Gtk::PACK_SHRINK);
	buttonBoxLogging_.pack_start(logButton3_, Gtk::PACK_SHRINK);
	buttonBoxLogging_.pack_start(logButton4_, Gtk::PACK_SHRINK);

	logView_.set_buffer(logBuffer_);
	logScrollWindow_.add(logView_);
	loggingTabPaned_.add1(logScrollWindow_);
	loggingTabPaned_.add2(buttonBoxLogging_);

	tabs_.append_page(playFieldTabGrid_, "Playfield");
	tabs_.append_page(loggingTabPaned_, "RefBox Log");
	add(tabs_);
	aspectFrame_.set(Gtk::ALIGN_START, Gtk::ALIGN_START, 1, true);
	playField_.set_size_request(600, 600);
	logScrollWindow_.set_size_request(600, 600);
	logPreviewScrollWindow_.set_size_request(0, 150);
	buttonBoxPlayField_.set_size_request(150, 0);
	buttonBoxLogging_.set_size_request(150, 0);
	show_all_children();

	stateWidget_.set_time(540);
	stateWidget_.set_score(543);

}

void MainWindow::add_log_message(std::string msg) {
	log_ += msg;
	logBuffer_->set_text(log_);
}

void MainWindow::set_score(int score) {

}

MainWindow::~MainWindow() {

}

} /* namespace LLSFVis */
