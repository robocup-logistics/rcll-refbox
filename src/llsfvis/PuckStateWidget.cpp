/*
 * PuckStateWidget.cpp
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#include "PuckStateWidget.h"
#include <sstream>

using namespace std;
using namespace llsf_msgs;

namespace LLSFVis {

PuckStateWidget::PuckStateWidget(int puck_id) {
	Glib::RefPtr<Gtk::StyleContext> stylecontext =
			state_label_.get_style_context();
	standard_fg_color_ = stylecontext->get_color();
	standard_bg_color_ = stylecontext->get_background_color();
	stringstream str("     "); //Allocate 5 Spaces as default size
	str << "P " <<puck_id << ":";
	id_label_.set_text(str.str());
	id_label_.set_justify(Gtk::JUSTIFY_LEFT);
	state_label_.set_text("--");
	id_label_.set_justify(Gtk::JUSTIFY_CENTER);
	pack_start(id_label_);
	pack_start(state_label_);
	show_all_children();
}

void PuckStateWidget::set_state(const llsf_msgs::PuckState& state) {
	printf("State: %s\n", PuckState_Name(state).c_str());
	state_label_.set_text(PuckState_Name(state));
	Pango::FontDescription font;
	Gdk::RGBA fg_color;
	Gdk::RGBA bg_color;

	switch (state) {
	case S0:
		//black on white
		fg_color = Gdk::RGBA("black");
		bg_color = standard_bg_color_;
		break;
	case S1:
		//white on green
		fg_color = Gdk::RGBA("white");
		bg_color = Gdk::RGBA("green");
		break;
	case S2:
		//white on yellow
		fg_color = Gdk::RGBA("white");
		bg_color = Gdk::RGBA("orange");
		break;
	case P1:
	case P2:
	case P3:
		//white on red
		fg_color = Gdk::RGBA("white");
		bg_color = Gdk::RGBA("red");
		break;
	case CONSUMED:
		state_label_.set_text("CO");
		fg_color = Gdk::RGBA("dark grey");
		bg_color = standard_bg_color_;
		//black on white, bold
		break;
	}
	state_label_.override_background_color(bg_color);
	state_label_.override_color(fg_color);
}

PuckStateWidget::~PuckStateWidget() {
}

} /* namespace LLSFVis */
