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

void RobotStatesWidget::update_bot(boost::posix_time::time_duration time_since_update, const llsf_msgs::Robot& bot,
		Gtk::Label* bot_label) {
	if (bot.IsInitialized()) {
		Gdk::RGBA color;

		boost::posix_time::ptime
		  now(boost::posix_time::microsec_clock::universal_time());
		boost::posix_time::ptime
		  last_seen(boost::posix_time::from_time_t(bot.last_seen().sec()));
		last_seen += boost::posix_time::nanoseconds(bot.last_seen().nsec());

		boost::posix_time::time_duration td = now - last_seen;
		long td_seconds = td.total_seconds();


		if (td_seconds >= TIMEEXCEEDED) {
			color = Gdk::RGBA("red");
		} else if (td_seconds >= TIMEWARNING) {
			color = Gdk::RGBA("yellow2");
		} else {
			color = Gdk::RGBA("black");
		}
		bot_label->override_color(color, Gtk::STATE_FLAG_NORMAL);
		char time_string[10];
		long int time_min = td_seconds / 60;
		long int time_sec = td_seconds % 60;
		sprintf(time_string,"%02li:%02li", time_min, time_sec);
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
	bot1_.CopyFrom(bot);
	bot1_updated = boost::posix_time::microsec_clock::local_time();
	bot1_frame_.set_label("[" + bot.team() + "]" + bot.name());
}

void RobotStatesWidget::setBot2(const llsf_msgs::Robot& bot) {
	bot2_.CopyFrom(bot);
	bot2_updated = boost::posix_time::microsec_clock::local_time();
	bot2_frame_.set_label("[" + bot.team() + "]" + bot.name());
}

void RobotStatesWidget::setBot3(const llsf_msgs::Robot& bot) {
	bot3_ .CopyFrom(bot);
	bot3_updated = boost::posix_time::microsec_clock::local_time();
	bot3_frame_.set_label("[" + bot.team() + "]" + bot.name());
}

void RobotStatesWidget::clear() {
	bot1_.Clear();
	bot2_.Clear();
	bot3_.Clear();
	bot1_label_.set_text("----");
	bot2_label_.set_text("----");
	bot3_label_.set_text("----");
}

} /* namespace LLSFVis */
