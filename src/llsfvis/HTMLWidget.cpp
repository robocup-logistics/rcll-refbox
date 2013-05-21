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
	htmlWidget_->set_size_request(300, 600);
	add(*htmlWidget_);
	show_all_children();
	Glib::signal_timeout().connect(
			sigc::mem_fun(*this, &HTMLWidget::switch_page), UPDATE_INTERVAL);
	add_page_to_display(RESDIR"/rules-1.html");
	add_page_to_display(RESDIR"/rules-2.html");
	add_page_to_display(RESDIR"/rules-3.html");
	add_page_to_display(RESDIR"/rules-4.html");
	page_index_ = 0;

}

HTMLWidget::~HTMLWidget() {
	// TODO Auto-generated destructor stub
}

bool HTMLWidget::switch_page() {

	display_page(pages[page_index_].c_str());
	page_index_ = (page_index_ + 1) % pages.size();
	return true;
}

void HTMLWidget::add_page_to_display(const gchar* uri) {
	std::string s = "file://";
	s += uri;
	pages.push_back(s);
}

void HTMLWidget::display_page(const gchar* uri) {
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(gtkhtml_), uri);
	htmlWidget_->set_size_request(300, 600);
}

} /* namespace LLSFVis */
