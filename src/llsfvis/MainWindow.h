/***************************************************************************
 *  MainWindow.h - Central GUI element for the LLSFVIS
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
#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <gtkmm.h>
#include "PlayField.h"
#include "StateWidget.h"

namespace LLSFVis {

class MainWindow: public Gtk::Window {
public:
	MainWindow();
	virtual ~MainWindow();
	void add_log_message(std::string msg);
	void set_score(int score);
private:
	Gtk::Notebook _tabs;
	Gtk::Grid _playFieldTabGrid;
	Gtk::Paned _loggingTabPaned;

	Gtk::AspectFrame _aspectFrame;
	PlayField _playField;
	Gtk::ScrolledWindow _logPreviewScrollWindow;
	Gtk::TextView _logPreview;

	Gtk::Box _buttonBoxPlayField;
	Gtk::Button _playFieldButton1;
	Gtk::Button _playFieldButton2;
	Gtk::Button _playFieldButton3;
	Gtk::Button _playFieldButton4;

	Gtk::Box _buttonBoxLogging;
	Gtk::Button _logButton1;
	Gtk::Button _logButton2;
	Gtk::Button _logButton3;
	Gtk::Button _logButton4;
	Gtk::ScrolledWindow _logScrollWindow;
	Gtk::TextView _logView;

	StateWidget _stateWidget;

	Glib::RefPtr<Gtk::TextBuffer> _logBuffer;
	std::string log;


};

} /* namespace LLSFVis */
#endif /* MAINWINDOW_H_ */
