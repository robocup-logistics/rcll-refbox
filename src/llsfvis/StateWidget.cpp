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

	scoreLabel_.set_justify(Gtk::JUSTIFY_CENTER);
	timeLabel_.set_justify(Gtk::JUSTIFY_CENTER);
	Pango::FontDescription font;
	font.set_size(Pango::SCALE * 32);
	font.set_weight(Pango::WEIGHT_BOLD);
	scoreLabel_.override_font(font);
	timeLabel_.override_font(font);
	scoreFrame_.set_label("Scores");
	timeFrame_.set_label("Game Time");

	timeFrame_.add(timeLabel_);
	scoreFrame_.add(scoreLabel_);
	pack_start(scoreFrame_, Gtk::PACK_EXPAND_WIDGET);
	pack_start(timeFrame_, Gtk::PACK_EXPAND_WIDGET);


}

void StateWidget::set_score(int score) {
	std::ostringstream os;
	os << score;
	scoreLabel_.set_text(os.str());
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
	timeLabel_.set_text(seconds_to_str(time));
}


StateWidget::~StateWidget() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
