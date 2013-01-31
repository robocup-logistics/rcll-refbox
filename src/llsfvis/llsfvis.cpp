/*
 * llsfvis.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */
#include <gtkmm.h>
#include "MainWindow.h"

//using namespace Gtk;
int main(int argc, char* argv[]) {
	Glib::RefPtr<Gtk::Application> app =
	    Gtk::Application::create(argc, argv,
	      "org.gtkmm.examples.base");

	  LLSFVis::MainWindow mainWindow;
	  return app->run(mainWindow);


}

