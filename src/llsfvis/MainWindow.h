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
#include "PlayFieldWidget.h"
#include "StateWidget.h"
#include "LogWidget.h"
#include "GameState.h"
#include "PlayField.h"

namespace LLSFVis {

class MainWindow: public Gtk::Window {
public:
	MainWindow();
	virtual ~MainWindow();
	void add_log_message(std::string msg);
	void update_game_state(GameState& gameState);
	void set_attention_msg(std::string msg);
	void set_playfield(PlayField& playField);
private:
	Gtk::Notebook tabs_;
	Gtk::Grid playFieldTabGrid_;
	Gtk::Paned loggingTabPaned_;

	Gtk::Label attentionMsg_;
	Gtk::AspectFrame aspectFrame_;
	PlayFieldWidget playFieldWidget_;
	Gtk::ScrolledWindow logPreviewScrollWindow_;
	LogWidget logPreviewWidget_;

	Gtk::Box buttonBoxPlayField_;
	Gtk::Button playFieldButton1_;
	Gtk::Button playFieldButton2_;
	Gtk::Button playFieldButton3_;
	Gtk::Button playFieldButton4_;

	Gtk::Box buttonBoxLogging_;
	Gtk::Button logButton1_;
	Gtk::Button logButton2_;
	Gtk::Button logButton3_;
	Gtk::Button logButton4_;
	Gtk::ScrolledWindow logScrollWindow_;

	StateWidget stateWidget_;

	LogWidget logWidget_;

};

} /* namespace LLSFVis */
#endif /* MAINWINDOW_H_ */
