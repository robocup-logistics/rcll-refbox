/*
 * RulesWidget.cpp
 *
 *  Created on: 14.05.2013
 *      Author: daniel
 */

#include "HTMLWidget.h"

#define UPDATE_INTERVAL 1000

using namespace std;

namespace LLSFVis {

HTMLWidget::HTMLWidget() {
	set_label("The Logistics League");
	gtkhtml_ = webkit_web_view_new();
	htmlWidget_ = Glib::wrap(gtkhtml_);
	htmlWidget_->set_size_request(300,600);
	add(*htmlWidget_);
	show_all_children();
	Glib::signal_timeout().connect(
				sigc::mem_fun(*this, &HTMLWidget::switch_page), UPDATE_INTERVAL);
	add_page_to_display("http://www.google.de");
	add_page_to_display("http://www.spiegel.de");
	add_page_to_display("http://www.yahoo.de");
	add_page_to_display("http://www.wtf.de");
	page_index_=0;

}


HTMLWidget::~HTMLWidget() {
	// TODO Auto-generated destructor stub
}

bool HTMLWidget::switch_page(){


	printf("Showing page %i\n",page_index_);
	display_page(pages[page_index_]);
	page_index_ = (page_index_+1)%pages.size();
	return true;
}

void HTMLWidget::add_page_to_display(const gchar* uri) {
	pages.push_back(uri);
}

void HTMLWidget::display_page(const gchar* uri) {
	 webkit_web_view_load_uri(WEBKIT_WEB_VIEW(gtkhtml_),uri);
	 htmlWidget_->set_size_request(300,600);
	 printf("done loading\n");
}

} /* namespace LLSFVis */
