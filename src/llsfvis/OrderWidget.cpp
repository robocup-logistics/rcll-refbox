/*
 * OrderWidget.cpp
 *
 *  Created on: 27.04.2013
 *      Author: daniel
 */

#include "OrderWidget.h"
#include "msgs/OrderInfo.pb.h"
#include "utils.h"
#include <sstream>

using namespace std;
using namespace llsf_msgs;
namespace LLSFVis {

OrderWidget::OrderWidget(int order_id) {
	stringstream str;
	str << "Order " << order_id;
	set_label(str.str());
	add(label_);
	show_all_children();
	Glib::RefPtr<Gtk::StyleContext> stylecontext = get_style_context();
	standard_bg_color_ = stylecontext->get_background_color();

}

OrderWidget::~OrderWidget() {
	// TODO Auto-generated destructor stub
}

void OrderWidget::update_order(const llsf_msgs::Order& order) {
	order_ = order;
	stringstream order_info;
	order_info << seconds_to_str(order.delivery_period_begin()) << " - "
			<< seconds_to_str(order.delivery_period_end()) << "\n Deliver "
			<< Order_ProductType_Name(order.product()) << " to "
			<< Order_DeliveryGate_Name(order.delivery_gate()) << " ("
			<< order.quantity_delivered() << " of "
			<< order.quantity_requested() << ").";
	label_.set_text(order_info.str());

}

void OrderWidget::set_game_time(const llsf_msgs::Time& time) {

	if (time.sec() > order_.delivery_period_begin()
			&& time.sec() < order_.delivery_period_end()) {
		label_.override_background_color(Gdk::RGBA("green"));
		label_.override_color(Gdk::RGBA("white"));
		override_background_color(Gdk::RGBA("green"));

	} else {
		label_.override_background_color(standard_bg_color_);
		label_.override_color(Gdk::RGBA("black"));
		override_background_color(standard_bg_color_);
	}

}

} /* namespace LLSFVis */
