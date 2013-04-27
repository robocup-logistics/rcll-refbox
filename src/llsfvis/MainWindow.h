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
#include "PucksWidget.h"
#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>
#include <msgs/AttentionMessage.pb.h>
#include <msgs/MachineInfo.pb.h>

namespace LLSFVis {

class MainWindow: public Gtk::Window {
public:
	MainWindow();
	virtual ~MainWindow();
	void add_log_message(std::string msg);
	void update_game_state(llsf_msgs::GameState& gameState);
	void update_robots(llsf_msgs::RobotInfo& robotInfo);
	void update_machines(llsf_msgs::MachineInfo& mSpecs);
	void update_pucks(const llsf_msgs::PuckInfo& pucks);
	void set_attention_msg(llsf_msgs::AttentionMessage& msg);

	sigc::signal<void, llsf_msgs::SetGameState&> signal_set_game_state() {
		return signal_set_game_state_;
	}

	sigc::signal<void, llsf_msgs::RemovePuckFromMachine&> signal_remove_puck() {
		return playFieldWidget_.signal_remove_puck();
	}

	sigc::signal<void, llsf_msgs::PlacePuckUnderMachine&> signal_place_puck_under_machine() {
		return signal_place_puck_under_machine_;
	}

private:

	bool clear_attention_msg();
	void on_add_puck_to_machine_button_clicked();
	void on_set_puck_under_rfid_button_clicked();
	void on_start_pause_button_clicked();
	llsf_msgs::PuckInfo*  find_free_pucks();

	Gtk::Notebook tabs_;
	Gtk::Grid playFieldTabGrid_;
	Gtk::Paned loggingTabPaned_;

	Gtk::Label attentionMsg_;
	Gtk::AspectFrame aspectFrame_;
	PlayFieldWidget playFieldWidget_;
	Gtk::ScrolledWindow logPreviewScrollWindow_;
	LogWidget logPreviewWidget_;

	Gtk::Box buttonBoxPlayField_;
	Gtk::Button addPuckToMachineButton_;
	Gtk::Button setPuckUnderRFIDButton_;
	Gtk::Button startPauseButton_;
	Gtk::Button playFieldButton4_;

	Gtk::Box buttonBoxLogging_;
	Gtk::Button logButton1_;
	Gtk::Button logButton2_;
	Gtk::Button logButton3_;
	Gtk::Button logButton4_;
	Gtk::ScrolledWindow logScrollWindow_;

	StateWidget stateWidget_;
	PucksWidget pucksWidget_;
	LogWidget logWidget_;

	const llsf_msgs::PuckInfo* pucks_ = NULL;
	llsf_msgs::PuckInfo* freePucks_ = NULL;
	const llsf_msgs::MachineInfo* machines_ = NULL;
	sigc::signal<void, llsf_msgs::SetGameState&> signal_set_game_state_;
	sigc::signal<void, llsf_msgs::PlacePuckUnderMachine&> signal_place_puck_under_machine_;

};

} /* namespace LLSFVis */
#endif /* MAINWINDOW_H_ */
