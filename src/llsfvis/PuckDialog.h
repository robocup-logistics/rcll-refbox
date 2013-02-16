/*
 * PuckDialog.h
 *
 *  Created on: 12.02.2013
 *      Author: daniel
 */

#ifndef PUCKDIALOG_H_
#define PUCKDIALOG_H_

#include <gtkmm.h>

namespace LLSFVis {

class PuckDialog: public Gtk::Dialog {
public:
	PuckDialog(const Glib::ustring & title);
	virtual ~PuckDialog();
	Glib::ustring get_value();
private:
	Gtk::Entry puck_Entry_;
};

} /* namespace LLSFVis */
#endif /* PUCKDIALOG_H_ */
