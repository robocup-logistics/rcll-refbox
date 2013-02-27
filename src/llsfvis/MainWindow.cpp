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

	//SETUP the GUI

	set_default_size(750, 750);
	set_position(Gtk::WIN_POS_CENTER);
	tabs_.set_border_width(10);

	//the PlayField tab
	playFieldButton1_.set_label("Button1");
	playFieldButton2_.set_label("Button2");
	playFieldButton3_.set_label("Button3");
	playFieldButton4_.set_label("Button4");

	buttonBoxPlayField_.pack_start(playFieldButton1_, Gtk::PACK_SHRINK);
	buttonBoxPlayField_.pack_start(playFieldButton2_, Gtk::PACK_SHRINK);
	buttonBoxPlayField_.pack_start(playFieldButton3_, Gtk::PACK_SHRINK);
	buttonBoxPlayField_.pack_start(playFieldButton4_, Gtk::PACK_SHRINK);

	logPreviewScrollWindow_.add(logPreviewWidget_);

	attentionMsg_.set_label("Attention:");

	Pango::FontDescription font;
	font.set_size(Pango::SCALE * 18);
	font.set_weight(Pango::WEIGHT_BOLD);
	attentionMsg_.override_color(Gdk::RGBA("dark red"));
	attentionMsg_.override_font(font);

	playFieldTabGrid_.set_row_spacing(5);
	playFieldTabGrid_.set_column_spacing(5);
	playFieldTabGrid_.attach(attentionMsg_, 0, 0, 2, 1);
	playFieldTabGrid_.attach_next_to(aspectFrame_, attentionMsg_,
			Gtk::POS_BOTTOM, 1, 2);
	playFieldTabGrid_.attach_next_to(buttonBoxPlayField_, aspectFrame_,
			Gtk::POS_RIGHT, 1, 1);
	playFieldTabGrid_.attach_next_to(logPreviewScrollWindow_, aspectFrame_,
			Gtk::POS_BOTTOM, 2, 1);
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

	//make both logviews use the same model
	logPreviewWidget_.set_model(logWidget_.get_model());
	logWidget_.set_hexpand(true);
	logPreviewWidget_.set_hexpand(true);
	logScrollWindow_.add(logWidget_);
	loggingTabPaned_.pack1(logScrollWindow_,true,true);
	loggingTabPaned_.pack2(buttonBoxLogging_,false,false);

	tabs_.append_page(playFieldTabGrid_, "Playfield");
	tabs_.append_page(loggingTabPaned_, "RefBox Log");
	add(tabs_);
	aspectFrame_.set(Gtk::ALIGN_START, Gtk::ALIGN_START, 1, true);
	aspectFrame_.add(playFieldWidget_);
	aspectFrame_.set_border_width(0);
	aspectFrame_.set_shadow_type(Gtk::SHADOW_NONE);
	playFieldWidget_.set_size_request(600, 600);
	playFieldWidget_.set_hexpand(true);
	playFieldWidget_.set_vexpand(true);
	logScrollWindow_.set_size_request(600, 600);
	logPreviewScrollWindow_.set_size_request(0, 150);
	buttonBoxPlayField_.set_size_request(150, 0);
	buttonBoxLogging_.set_size_request(150, 0);
	show_all_children();
	stateWidget_.set_time(540);
	stateWidget_.set_score(543);

}

void MainWindow::add_log_message(std::string msg) {
	logWidget_.add_log_message(msg);
	//logPreviewWidget_.add_log_message(msg);
}

MainWindow::~MainWindow() {

}

void MainWindow::update_game_state(llsf_msgs::GameState& gameState) {
	//playFieldWidget_.update_game_state(gameState);
	stateWidget_.update_game_state(gameState);
}

void MainWindow::set_attention_msg(std::string msg) {
	attentionMsg_.set_text(msg);
}

void MainWindow::update_robots(llsf_msgs::RobotInfo& robotInfo) {
	stateWidget_.update_robot_info(robotInfo);
	playFieldWidget_.update_robot_info(robotInfo);
}

} /* namespace LLSFVis */
