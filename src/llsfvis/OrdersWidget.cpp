/*
 * OrderWidget.cpp
 *
 *  Created on: 27.04.2013
 *      Author: daniel
 */

#include "OrdersWidget.h"

using namespace Gtk;

namespace LLSFVis {

OrdersWidget::OrdersWidget() :
		orders_box_(ORIENTATION_VERTICAL) {

	set_label("Orders");
	set_size_request(200, 0);
	add(orders_box_);
}

void OrdersWidget::update_orders(const llsf_msgs::OrderInfo& orderInfo) {
	for (int i = 0; i < orderInfo.orders_size(); ++i) {
		if (orders_.find(orderInfo.orders(i).id()) != orders_.end()) {
			update_order(orderInfo.orders(i));
		} else {
			add_order(orderInfo.orders(i));
		}
	}
}

void OrdersWidget::add_order(const llsf_msgs::Order& order) {
	OrderWidget* ow = new OrderWidget(order.id());
	ow->update_order(order);
	orders_[order.id()] = ow;
	orders_box_.pack_start(*ow, Gtk::PACK_SHRINK);
	show_all_children();
}

void OrdersWidget::update_order(const llsf_msgs::Order& order) {
	orders_[order.id()]->update_order(order);
}

void OrdersWidget::remove_order(const llsf_msgs::Order& order) {
	OrderWidget* o = orders_[order.id()];
	orders_box_.remove(*o);
	orders_.erase(order.id());
	delete o;
}

void OrdersWidget::update_game_time(const llsf_msgs::Time& time) {
	time_=time;
	orders_box_.foreach(sigc::mem_fun(*this,&OrdersWidget::set_game_time)); //TODO hier weiter
}

void OrdersWidget::set_game_time(Gtk::Widget& w){
	OrderWidget& ow = dynamic_cast<OrderWidget&>(w);
	ow.set_game_time(time_);
}


OrdersWidget::~OrdersWidget() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
