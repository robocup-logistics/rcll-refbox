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
	set_game_phase(GameState::PRESTART);

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

void StateWidget::set_game_phase(GameState::GamePhase gamePhase) {
	Glib::ustring label;
	switch (gamePhase) {
	case GameState::PRESTART:
		label = "Preparation";
		break;
	case GameState::EXPLORE:
		label = "Exploration";
		break;
	case GameState::PRODUCE:
		label = "Production";
		break;
	case GameState::END:
		label = "Game Over";
		break;
	case GameState::PAUSE:
		label = "Game Paused";
		break;
	default:
		break;
	}
	gameStateLabel_.set_text(label);
}

void StateWidget::update_game_state(GameState& gameState) {
	const std::list<const Robot*>& bots = gameState.getRobots();
	std::vector<const Robot*> botsv(bots.begin(), bots.end());
	switch (botsv.size()) {
	case 3:
		botStates_.setBot3(botsv.at(2));
	case 2:
		botStates_.setBot2(botsv.at(1));
	case 1:
		botStates_.setBot1(botsv.at(0));
		break;
	default:
		botStates_.clear();
	}
	set_time(gameState.getTime());
	set_score(gameState.getScore());

}

} /* namespace LLSFVis */

