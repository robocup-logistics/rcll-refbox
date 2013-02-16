/*
 * PuckDialog.cpp
 *
 *  Created on: 12.02.2013
 *      Author: daniel
 */

#include "PuckDialog.h"

namespace LLSFVis {

PuckDialog::PuckDialog(const Glib::ustring & title) {
	set_title(title);
	puck_Entry_.set_size_request(200);
	//get_content_area()->pack_start(puck_Entry_);

	add_action_widget(puck_Entry_,Gtk::RESPONSE_OK);
	add_button(Gtk::Stock::OK,Gtk::RESPONSE_OK);
	add_button(Gtk::Stock::CANCEL,Gtk::RESPONSE_CANCEL);
	show_all_children();
}

PuckDialog::~PuckDialog() {
	// TODO Auto-generated destructor stub
}

Glib::ustring PuckDialog::get_value() {
	return puck_Entry_.get_text();
}
} /* namespace LLSFVis */
;
