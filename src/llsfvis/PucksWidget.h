/*
 * PucksWidget.h
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#ifndef PUCKSWIDGET_H_
#define PUCKSWIDGET_H_

#include <gtkmm.h>
#include <map>
#include "PuckStateWidget.h"
#include "msgs/PuckInfo.pb.h"

namespace LLSFVis {

class PucksWidget: public Gtk::Frame {
public:
	PucksWidget();
	virtual ~PucksWidget();
	void add_puck(const llsf_msgs::Puck& puck);
	void remove_puck(const llsf_msgs::Puck& puck);
	void update_puck(const llsf_msgs::Puck& puck);
	void update_pucks(const llsf_msgs::PuckInfo& pucks);

private:
	Gtk::Box box_;
	std::map<int,PuckStateWidget*> pucks_;
};

} /* namespace LLSFVis */
#endif /* PUCKSWIDGET_H_ */
