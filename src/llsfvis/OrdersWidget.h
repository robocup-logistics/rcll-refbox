/*
 * OrdersWidget.h
 *
 *  Created on: 27.04.2013
 *      Author: daniel
 */

#ifndef ORDERSWIDGET_H_
#define ORDERSWIDGET_H_

#include "msgs/OrderInfo.pb.h"
#include "msgs/Time.pb.h"
#include "OrderWidget.h"
#include <map>
#include <gtkmm.h>

namespace LLSFVis {

class OrdersWidget : public Gtk::Frame {
public:
	OrdersWidget();
	void update_orders(const llsf_msgs::OrderInfo& orderInfo);
	void update_game_time(const llsf_msgs::Time& time);
	void add_order(const llsf_msgs::Order& order);
	void update_order(const llsf_msgs::Order& order);
	void remove_order(const llsf_msgs::Order& order);

	virtual ~OrdersWidget();
private:
	void set_game_time(Gtk::Widget& w);
private:
	llsf_msgs::Time time_;
	std::map<int,OrderWidget*> orders_;
	Gtk::Box orders_box_;
};

} /* namespace LLSFVis */
#endif /* ORDERSWIDGET_H_ */
