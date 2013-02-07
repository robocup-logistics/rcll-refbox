/*
 * ScoreView.h
 *
 *  Created on: 06.02.2013
 *      Author: daniel
 */

#ifndef SCOREVIEW_H_
#define SCOREVIEW_H_

#include <gtkmm.h>

namespace LLSFVis {

class StateWidget: public Gtk::Box {
public:
	StateWidget();
	virtual ~StateWidget();
	void set_score(int score);
	void set_time(int score);
private:
	Gtk::Label timeLabel_;
	Gtk::Label scoreLabel_;
	Gtk::Frame timeFrame_;
	Gtk::Frame scoreFrame_;
};

} /* namespace LLSFVis */
#endif /* SCOREVIEW_H_ */
