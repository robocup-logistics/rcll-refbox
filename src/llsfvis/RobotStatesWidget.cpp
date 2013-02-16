/*
 * RobotStatesWidget.cpp
 *
 *  Created on: 11.02.2013
 *      Author: daniel
 */

#include "RobotStatesWidget.h"
#include <iostream>

namespace LLSFVis {

RobotStatesWidget::RobotStatesWidget() :
		box_(Gtk::ORIENTATION_VERTICAL) {
	bot1_ = NULL;
	bot2_ = NULL;
	bot3_ = NULL;

	Pango::FontDescription font;
	font.set_size(Pango::SCALE * 28);
	font.set_weight(Pango::WEIGHT_BOLD);
	bot1_label_.override_font(font);
	bot2_label_.override_font(font);
	bot3_label_.override_font(font);

	bot1_frame_.add(bot1_label_);
	bot2_frame_.add(bot2_label_);
	bot3_frame_.add(bot3_label_);

	bot1_label_.set_text("----");
	bot2_label_.set_text("----");
	bot3_label_.set_text("----");

	box_.pack_start(bot1_frame_);
	box_.pack_start(bot2_frame_);
	box_.pack_start(bot3_frame_);

	set_label("Robots");
	add(box_);
	Glib::signal_timeout().connect(
			sigc::mem_fun(*this, &RobotStatesWidget::on_update), 1000);
}

void RobotStatesWidget::update_bot(time_t rawtime, const Robot* bot,
		Gtk::Label* bot_label) {
	if (bot != NULL) {
		time_t bot_lastping = rawtime - bot->getLastHeard();
		char bot_time[6];
		strftime(bot_time, 6, "%M:%S", localtime(&bot_lastping));
		Gdk::RGBA color;
		int secondsGone = (localtime(&bot_lastping))->tm_sec;
		if (secondsGone > TIMEEXCEEDED) {
			color = Gdk::RGBA("dark red");
		} else if (secondsGone > TIMEWARNING) {
			color = Gdk::RGBA("yellow2");
		} else {
			color = Gdk::RGBA("black");
		}
		bot_label->override_color(color, Gtk::STATE_FLAG_NORMAL);
		bot_label->set_text(bot_time);
	}
}

bool RobotStatesWidget::on_update() {
	time_t rawtime;
	time(&rawtime);
	update_bot(rawtime, bot1_, &bot1_label_);
	update_bot(rawtime, bot2_, &bot2_label_);
	update_bot(rawtime, bot3_, &bot3_label_);

	//return event is handled
	return true;
}

RobotStatesWidget::~RobotStatesWidget() {
	// TODO Auto-generated destructor stub
}

void RobotStatesWidget::setBot1(const Robot* bot) {
	bot1_ = bot;
	bot1_frame_.set_label(bot->getId());
}

void RobotStatesWidget::setBot2(const Robot* bot) {
	bot2_ = bot;
	bot2_frame_.set_label(bot->getId());
}

void RobotStatesWidget::setBot3(const Robot* bot) {
	bot3_ = bot;
	bot3_frame_.set_label(bot->getId());
}

void RobotStatesWidget::clear() {
	bot1_ = NULL;
	bot2_ = NULL;
	bot3_ = NULL;
	bot1_label_.set_text("----");
	bot2_label_.set_text("----");
	bot3_label_.set_text("----");
}

} /* namespace LLSFVis */
