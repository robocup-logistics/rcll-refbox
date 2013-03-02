/*
 * PlacePuckDialog.cpp
 *
 *  Created on: 02.03.2013
 *      Author: daniel
 */

#include "PlacePuckDialog.h"
#include <sstream>
using namespace std;
using namespace llsf_msgs;

namespace LLSFVis {

PlacePuckDialog::PlacePuckDialog(vector<Puck*>& pucks,
		vector<Machine*>& machines) :
		pucks_(pucks), machines_(machines) {

	set_title("Select puck and machine to fill puck with");

	get_content_area()->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	get_content_area()->pack_start(combo_pucks_, Gtk::PACK_SHRINK);
	get_content_area()->pack_start(combo_machines_, Gtk::PACK_SHRINK);

	add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	for (vector<Puck*>::iterator it = pucks.begin(); it != pucks.end(); ++it) {
		stringstream s;
		s << (*it)->id() << ": " << PuckState_Name((*it)->state());
		combo_pucks_.append(s.str());
	}

	for (vector<Machine*>::iterator it = machines.begin(); it != machines.end();
			++it) {
		stringstream s;
		s << (*it)->name() << ": " << (*it)->type();
		combo_machines_.append(s.str());
	}
	combo_pucks_.set_active(0);
	combo_machines_.set_active(0);
	show_all_children();
}

PlacePuckDialog::~PlacePuckDialog() {
	// TODO Auto-generated destructor stub
}

llsf_msgs::Machine* PlacePuckDialog::get_selected_machine() {
	return machines_.at(combo_machines_.get_active_row_number());
}

llsf_msgs::Puck* PlacePuckDialog::get_selected_puck() {
	return pucks_.at(combo_pucks_.get_active_row_number());
}

} /* namespace LLSFVis */
