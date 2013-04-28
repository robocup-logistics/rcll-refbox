/*
 * OrderWidget.h
 *
 *  Created on: 27.04.2013
 *      Author: daniel
 */

#ifndef ORDERWIDGET_H_
#define ORDERWIDGET_H_

#include <gtkmm.h>
#include "msgs/OrderInfo.pb.h"
#include "msgs/Time.pb.h"

namespace LLSFVis {

class OrderWidget : public Gtk::Frame {
public:
	OrderWidget(int order_id);
	virtual ~OrderWidget();
	void update_order(const llsf_msgs::Order& order);
	void set_game_time(const llsf_msgs::Time& time);
private:
	Gtk::Label label_;
	llsf_msgs::Order order_;
	Gdk::RGBA standard_bg_color_;
};

} /* namespace LLSFVis */
#endif /* ORDERWIDGET_H_ */
