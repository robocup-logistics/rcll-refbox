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

PlacePuckDialog::PlacePuckDialog(const llsf_msgs::PuckInfo& pucks,
		const llsf_msgs::MachineInfo& machines) :
		pucks_(pucks), machines_(machines) {

	set_title("Select puck and machine to fill puck with");

	get_content_area()->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	get_content_area()->pack_start(combo_pucks_, Gtk::PACK_SHRINK);
	get_content_area()->pack_start(combo_machines_, Gtk::PACK_SHRINK);

	add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	for (int i =0; i < pucks_.pucks_size(); ++i) {
		stringstream s;
		s << pucks_.pucks(i).id() << ": " << PuckState_Name(pucks_.pucks(i).state());
		combo_pucks_.append(s.str());
	}

	for (int i = 0; i < machines_.machines_size(); ++i) {
		stringstream s;
		s << machines_.machines(i).name() << ": " << machines_.machines(i).type();
		combo_machines_.append(s.str());
	}
	combo_pucks_.set_active(0);
	combo_machines_.set_active(0);
	show_all_children();
}

PlacePuckDialog::~PlacePuckDialog() {
	// TODO Auto-generated destructor stub
}

const llsf_msgs::Machine& PlacePuckDialog::get_selected_machine() {
	return machines_.machines(combo_machines_.get_active_row_number());
}

const llsf_msgs::Puck& PlacePuckDialog::get_selected_puck() {
	return pucks_.pucks(combo_pucks_.get_active_row_number());
}

} /* namespace LLSFVis */
