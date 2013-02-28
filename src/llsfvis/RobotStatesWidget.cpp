/*
 * RobotStatesWidget.cpp
 *
 *  Created on: 11.02.2013
 *      Author: daniel
 */

#include "RobotStatesWidget.h"
#include <iostream>


#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

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

void RobotStatesWidget::update_bot(boost::posix_time::time_duration time_since_update, const llsf_msgs::Robot* bot,
		Gtk::Label* bot_label) {
	if (bot != NULL) {
		Gdk::RGBA color;

		boost::posix_time::seconds last_seen(bot->last_seen().sec());
		time_since_update += last_seen;

		if (time_since_update.seconds() > TIMEEXCEEDED) {
			color = Gdk::RGBA("dark red");
		} else if (time_since_update.seconds() > TIMEWARNING) {
			color = Gdk::RGBA("yellow2");
		} else {
			color = Gdk::RGBA("black");
		}
		bot_label->override_color(color, Gtk::STATE_FLAG_NORMAL);
		char time_string[10];
		sprintf(time_string,"%02d:%02d", time_since_update.minutes(),time_since_update.seconds());
		bot_label->set_text(time_string);
	}
}

bool RobotStatesWidget::on_update() {
	boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
	update_bot(now - bot1_updated, bot1_, &bot1_label_);
	update_bot(now - bot2_updated, bot2_, &bot2_label_);
	update_bot(now - bot2_updated, bot3_, &bot3_label_);

	//return event is handled
	return true;
}

RobotStatesWidget::~RobotStatesWidget() {
	// TODO Auto-generated destructor stub
}

void RobotStatesWidget::setBot1(const llsf_msgs::Robot& bot) {
	bot1_ = &bot;
	bot1_updated = boost::posix_time::microsec_clock::local_time();
	bot1_frame_.set_label("[" + bot.team() + "]" + bot.name());
}

void RobotStatesWidget::setBot2(const llsf_msgs::Robot& bot) {
	bot2_ = &bot;
	bot2_updated = boost::posix_time::microsec_clock::local_time();
	bot2_frame_.set_label("[" + bot.team() + "]" + bot.name());
}

void RobotStatesWidget::setBot3(const llsf_msgs::Robot& bot) {
	bot3_ = &bot;
	bot3_updated = boost::posix_time::microsec_clock::local_time();
	bot3_frame_.set_label("[" + bot.team() + "]" + bot.name());
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
