/*
 * PucksWidget.cpp
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#include "PucksWidget.h"
#include <map>


namespace LLSFVis {

using namespace llsf_msgs;
using namespace std;

PucksWidget::PucksWidget():
	box_(Gtk::ORIENTATION_VERTICAL){
	set_size_request(100,10);
	set_label("Puck States");
	add(box_);

}

PucksWidget::~PucksWidget() {
	for ( std::map<int,PuckStateWidget*>::iterator it = pucks_.begin(); it!=pucks_.end();++it){
		delete it->second;
	}
}

void PucksWidget::add_puck(const Puck& puck) {
	PuckStateWidget* pw = new PuckStateWidget(puck.id());
	pw->set_state(puck.state());
	pucks_[puck.id()] = pw;
	box_.pack_start(*pw, Gtk::PACK_SHRINK);
	show_all_children();
}

void PucksWidget::remove_puck(const Puck& puck){
	PuckStateWidget* p = pucks_[puck.id()];
	box_.remove(*p);
	pucks_.erase(puck.id());
	delete p;
}

void PucksWidget::update_puck(const Puck& puck){
	pucks_[puck.id()]->set_state(puck.state());
}

void PucksWidget::update_pucks(const PuckInfo& pucks) {
	for (int i = 0; i < pucks.pucks_size(); ++i) {
		if (pucks_.find(pucks.pucks(i).id()) != pucks_.end()) {
			update_puck(pucks.pucks(i));
		} else {
			add_puck(pucks.pucks(i));
		}
	}
}



} /* namespace LLSFVis */
