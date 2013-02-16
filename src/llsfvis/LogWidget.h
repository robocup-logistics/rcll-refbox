/*
 * LogWidget.h
 *
 *  Created on: 16.02.2013
 *      Author: daniel
 */

#ifndef LOGWIDGET_H_
#define LOGWIDGET_H_

#include <gtkmm.h>

namespace LLSFVis {

class LogWidget: public Gtk::TreeView {
	class Columns: public Gtk::TreeModel::ColumnRecord {
	public:
		Columns() {
			add(col_entry_);
		}

		~Columns() {
		}

		Gtk::TreeModelColumn<Glib::ustring> col_entry_;
	};
public:
	LogWidget();
	void add_log_message(std::string msg);
	virtual ~LogWidget();

private:
	Columns cols_;

};

} /* namespace LLSFVis */
#endif /* LOGWIDGET_H_ */
