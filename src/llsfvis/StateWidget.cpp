/*
 * ScoreView.cpp
 *
 *  Created on: 06.02.2013
 *      Author: daniel
 */

#include "StateWidget.h"
#include <iostream>

namespace LLSFVis {

StateWidget::StateWidget() {

	set_orientation(Gtk::ORIENTATION_VERTICAL);

	_scoreLabel.set_justify(Gtk::JUSTIFY_CENTER);
	_timeLabel.set_justify(Gtk::JUSTIFY_CENTER);
	Pango::FontDescription font;
	font.set_size(Pango::SCALE * 32);
	font.set_weight(Pango::WEIGHT_BOLD);
	_scoreLabel.override_font(font);
	_timeLabel.override_font(font);
	_scoreFrame.set_label("Scores");
	_timeFrame.set_label("Game Time");

	_timeFrame.add(_timeLabel);
	_scoreFrame.add(_scoreLabel);
	pack_start(_scoreFrame, Gtk::PACK_EXPAND_WIDGET);
	pack_start(_timeFrame, Gtk::PACK_EXPAND_WIDGET);


}

void StateWidget::set_score(int score) {
	std::ostringstream os;
	os << score;
	_scoreLabel.set_text(os.str());
}

std::string seconds_to_str(int sec) {
	int minutes = sec / 60;
	int seconds = sec % 60;
	std::ostringstream os;
	std::string sec_zeroes = "";
	std::string min_zeroes = "";
	if (seconds < 10) {
		sec_zeroes = "0";
	}
	if (minutes < 10) {
		min_zeroes = "0";
	}
	os << min_zeroes << minutes << ":" << sec_zeroes << seconds;
	return os.str();
}

void StateWidget::set_time(int time) {
	_timeLabel.set_text(seconds_to_str(time));
}


StateWidget::~StateWidget() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
