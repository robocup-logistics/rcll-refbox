/*
 * RulesWidget.h
 *
 *  Created on: 14.05.2013
 *      Author: daniel
 */

#ifndef RULESWIDGET_H_
#define RULESWIDGET_H_

#include <gtkmm.h>
#include <string>
#include <vector>

extern "C" {
#include <webkit/webkitwebview.h>
}


namespace LLSFVis {

class HTMLWidget: public Gtk::Frame {
public:
	HTMLWidget();
	virtual ~HTMLWidget();
	void add_page_to_display(const gchar* uri);
private:
	int page_index_;
	bool switch_page();
	GtkWidget* gtkhtml_;
	Gtk::Widget* htmlWidget_;
	void display_page(const gchar* uri);
	std::vector<std::string	> pages;

};

} /* namespace LLSFVis */
#endif /* RULESWIDGET_H_ */
