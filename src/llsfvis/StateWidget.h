/*
 * ScoreView.h
 *
 *  Created on: 06.02.2013
 *      Author: daniel
 */

#ifndef SCOREVIEW_H_
#define SCOREVIEW_H_

#include <gtkmm.h>
#include "RobotStatesWidget.h"
#include <msgs/GameState.pb.h>
#include <msgs/RobotInfo.pb.h>
namespace LLSFVis {



class StateWidget: public Gtk::Box {
public:



	StateWidget();
	virtual ~StateWidget();
	void set_score(int score);
	void set_time(const llsf_msgs::Time& time);
	void set_game_phase(llsf_msgs::GameState::Phase phase);
	void set_game_state(llsf_msgs::GameState::State state);
	void update_game_state(llsf_msgs::GameState& gameState);
	void update_robot_info(llsf_msgs::RobotInfo& robotInfo);
private:
	Gtk::Label timeLabel_;
	Gtk::Frame timeFrame_;

	Gtk::Label scoreLabel_;
	Gtk::Frame scoreFrame_;

	Gtk::Label gamePhaseLabel_;
	Gtk::Frame gamePhaseFrame_;

	Gtk::Frame gameStateFrame_;
	Gtk::Label gameStateLabel_;

	RobotStatesWidget botStates_;


};

} /* namespace LLSFVis */
#endif /* SCOREVIEW_H_ */
