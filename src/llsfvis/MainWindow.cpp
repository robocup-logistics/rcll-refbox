/*
 * MainWindow.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include "MainWindow.h"
#include <iostream>

namespace LLSFVis {

MainWindow::MainWindow() {

	set_default_size(1000, 800);
	set_position(Gtk::WIN_POS_CENTER);
	_vPaned = new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL);
	add(*_vPaned);
	_playField = new PlayField();

	Machine* m = new Machine(100, 100, Machine::NORTH, "bla");
	Puck* p = new Puck(100,150,"12","raw");

	_playField->add_machine(m);
	_playField->add_puck(p);
	_buttonBox = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
	_button1 = new Gtk::Button("Button1");
	_button2 = new Gtk::Button("Button2");
	_button3 = new Gtk::Button("Button3");
	_button4 = new Gtk::Button("Button4");
	_buttonBox->pack_start(*_button1, Gtk::PACK_SHRINK);
	_buttonBox->pack_start(*_button2, Gtk::PACK_SHRINK);
	_buttonBox->pack_start(*_button3, Gtk::PACK_SHRINK);
	_buttonBox->pack_start(*_button4, Gtk::PACK_SHRINK);

	//_vPaned->set
	_vPaned->add1(*_playField);
	_vPaned->add2(*_buttonBox);

	_button1->show();
	_button2->show();
	_button3->show();
	_button4->show();

	_buttonBox->show();
	_playField->show();
	_vPaned->show();
	show();

}

MainWindow::~MainWindow() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
