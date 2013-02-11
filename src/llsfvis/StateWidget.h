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

namespace LLSFVis {
enum GameState{
	PRESTART,
	EXPLORE,
	PRODUCE,
	END,
	PAUSE
};

class StateWidget: public Gtk::Box {
public:
	StateWidget();
	virtual ~StateWidget();
	void set_score(int score);
	void set_time(int score);
	void set_game_state(GameState gameState);
private:
	Gtk::Label timeLabel_;
	Gtk::Frame timeFrame_;

	Gtk::Label scoreLabel_;
	Gtk::Frame scoreFrame_;

	Gtk::Frame gameStateFrame_;
	Gtk::Label gameStateLabel_;

	RobotStatesWidget botStates_;


};

} /* namespace LLSFVis */
#endif /* SCOREVIEW_H_ */
