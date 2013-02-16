/*
 * GameState.h
 *
 *  Created on: 11.02.2013
 *      Author: daniel
 */

#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#include "Puck.h"
#include "Machine.h"
#include "Robot.h"

namespace LLSFVis {

class GameState {

public:
	enum GamePhase {
		PRESTART, EXPLORE, PRODUCE, END, PAUSE
	};

	const std::list<const Machine*>& getMachines() const {
		return machines_;
	}

	void setMachines(const std::list<const Machine*>& machines) {
		machines_ = machines;
	}

	GamePhase getPhase() const {
		return phase_;
	}

	void setPhase(GamePhase phase) {
		phase_ = phase;
	}

	const std::list<const Puck*>& getPucks() const {
		return pucks_;
	}

	void setPucks(const std::list<const Puck*>& pucks) {
		pucks_ = pucks;
	}

	const std::list<const Robot*>& getRobots() const {
		return robots_;
	}

	void setRobots(const std::list<const Robot*>& robots) {
		robots_ = robots;
	}

	int getScore() const {
		return score_;
	}

	void setScore(int score) {
		score_ = score;
	}

	time_t getTime() const {
		return time_;
	}

	void setTime(time_t time) {
		time_ = time;
	}

private:
	time_t time_;
	int score_;
	GamePhase phase_;
	std::list<const Robot*> robots_;
	std::list<const Puck*> pucks_;
	std::list<const Machine*> machines_;

};
}/* namespace LLSFVis */
#endif /* GAMESTATE_H_ */
