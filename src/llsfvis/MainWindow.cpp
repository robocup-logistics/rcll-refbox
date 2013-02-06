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
		_loggingTabPaned(Gtk::ORIENTATION_HORIZONTAL), _buttonBoxPlayField(
				Gtk::ORIENTATION_VERTICAL), _buttonBoxLogging(
				Gtk::ORIENTATION_VERTICAL) {

	//INITIALISE DATA
	_logBuffer = Gtk::TextBuffer::create();
	Machine* m = new Machine(1.2, 1.0, Machine::NORTH, "machine_1\ntype: T1");
	Puck* p = new Puck(2.5, 2.5, "12", "raw");


	//SETUP the GUI

	set_default_size(750, 750);
	set_position(Gtk::WIN_POS_CENTER);
	_tabs.set_border_width(10);

	//the PlayField tab


	_playField.add_machine(m);
	_playField.add_puck(p);
	_playFieldButton1.set_label("Button1");
	_playFieldButton2.set_label("Button2");
	_playFieldButton3.set_label("Button3");
	_playFieldButton4.set_label("Button4");

	_buttonBoxPlayField.pack_start(_playFieldButton1, Gtk::PACK_SHRINK);
	_buttonBoxPlayField.pack_start(_playFieldButton2, Gtk::PACK_SHRINK);
	_buttonBoxPlayField.pack_start(_playFieldButton3, Gtk::PACK_SHRINK);
	_buttonBoxPlayField.pack_start(_playFieldButton4, Gtk::PACK_SHRINK);

	_logPreviewScrollWindow.add(_logPreview);
	_logPreview.set_buffer(_logBuffer);

	_playFieldTabGrid.set_row_spacing(5);
	_playFieldTabGrid.set_column_spacing(5);
	_playFieldTabGrid.add(_playField);
	_playFieldTabGrid.attach_next_to(_buttonBoxPlayField, _playField,
			Gtk::POS_RIGHT, 1, 1);
	_playFieldTabGrid.attach_next_to(_logPreviewScrollWindow, _playField,
			Gtk::POS_BOTTOM, 1, 1);
	_playFieldTabGrid.attach_next_to(_stateWidget,_buttonBoxPlayField,Gtk::POS_BOTTOM,1,1);

	//Create the logging tab

	_logButton1.set_label("log1");
	_logButton2.set_label("log2");
	_logButton3.set_label("log3");
	_logButton4.set_label("log4");

	_buttonBoxLogging.pack_start(_logButton1, Gtk::PACK_SHRINK);
	_buttonBoxLogging.pack_start(_logButton2, Gtk::PACK_SHRINK);
	_buttonBoxLogging.pack_start(_logButton3, Gtk::PACK_SHRINK);
	_buttonBoxLogging.pack_start(_logButton4, Gtk::PACK_SHRINK);

	_logView.set_buffer(_logBuffer);
	_logScrollWindow.add(_logView);
	_loggingTabPaned.add1(_logScrollWindow);
	_loggingTabPaned.add2(_buttonBoxLogging);

	_tabs.append_page(_playFieldTabGrid, "Playfield");
	_tabs.append_page(_loggingTabPaned, "RefBox Log");
	add(_tabs);
	_aspectFrame.set(Gtk::ALIGN_START, Gtk::ALIGN_START, 1, true);
	_playField.set_size_request(600, 600);
	_logScrollWindow.set_size_request(600, 600);
	_logPreviewScrollWindow.set_size_request(0, 150);
	_buttonBoxPlayField.set_size_request(150, 0);
	_buttonBoxLogging.set_size_request(150, 0);
	show_all_children();

	_stateWidget.set_time(540);
	_stateWidget.set_score(543);

}

void MainWindow::add_log_message(std::string msg){
	log+=msg;
	_logBuffer->set_text(log);
}

void MainWindow::set_score(int score){

}

MainWindow::~MainWindow(){

}

} /* namespace LLSFVis */
