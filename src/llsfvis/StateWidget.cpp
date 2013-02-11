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
	font.set_size(Pango::SCALE * 28);
	font.set_weight(Pango::WEIGHT_BOLD);
	scoreLabel_.override_font(font);
	timeLabel_.override_font(font);

	gameStateLabel_.override_font(font);
	gameStateFrame_.set_label("Game State");
	scoreFrame_.set_label("Scores");
	timeFrame_.set_label("Game Time");


	timeFrame_.add(timeLabel_);
	scoreFrame_.add(scoreLabel_);
	gameStateFrame_.add(gameStateLabel_);

	pack_start(gameStateFrame_);
	pack_start(botStates_);
	pack_start(scoreFrame_, Gtk::PACK_EXPAND_WIDGET);
	pack_start(timeFrame_, Gtk::PACK_EXPAND_WIDGET);
	set_game_state(LLSFVis::PRESTART);

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

void StateWidget::set_game_state(GameState gameState) {
	Glib::ustring label;
	switch (gameState) {
		case PRESTART:
			label = "Preparation";
			break;
		case EXPLORE:
			label = "Exploration";
			break;
		case PRODUCE:
			label = "Production";
			break;
		case END:
			label = "Game Over";
			break;
		case PAUSE:
			label = "Game Paused";
			break;
		default:
			break;
	}
	gameStateLabel_.set_text(label);
}

} /* namespace LLSFVis */

