/*
 * PuckStateWidget.h
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#ifndef PUCKSTATEWIDGET_H_
#define PUCKSTATEWIDGET_H_

#include <gtkmm.h>
#include "msgs/PuckInfo.pb.h"


namespace LLSFVis {

class PuckStateWidget : public Gtk::Box {
public:
	PuckStateWidget(int puck_id);
	virtual ~PuckStateWidget();
	void set_state(const llsf_msgs::PuckState& state);


private:
	Gdk::RGBA standard_fg_color_;
	Gdk::RGBA standard_bg_color_;

	Gtk::Label id_label_;
	Gtk::Label state_label_;
	void update_label_text();
};

} /* namespace LLSFVis */
#endif /* PUCKSTATEWIDGET_H_ */
