/*
 * MainWindow.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <gtkmm.h>
#include "PlayField.h"

namespace LLSFVis {

class MainWindow: public Gtk::Window {
public:
	MainWindow();
	virtual ~MainWindow();
private:
	Gtk::Paned* _vPaned;

	PlayField* _playField;

	Gtk::Box* _buttonBox;

	Gtk::Button* _button1;
	Gtk::Button* _button2;
	Gtk::Button* _button3;
	Gtk::Button* _button4;


};

} /* namespace LLSFVis */
#endif /* MAINWINDOW_H_ */
