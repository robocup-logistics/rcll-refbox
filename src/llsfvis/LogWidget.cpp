/*
 * LogWidget.cpp
 *
 *  Created on: 16.02.2013
 *      Author: daniel
 */

#include "LogWidget.h"

namespace LLSFVis {

LogWidget::LogWidget() {
	set_model(Gtk::ListStore::create(cols_));
	append_column("Log Entries", cols_.col_entry_);
	show_all_children();

}

void LogWidget::add_log_message(std::string msg) {
	Glib::RefPtr<Gtk::ListStore> ls = Glib::RefPtr<Gtk::ListStore>::cast_static(
			get_model());
	Gtk::TreeModel::Row row = (*ls->append());
	row[cols_.col_entry_] = msg;
}

LogWidget::~LogWidget() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
