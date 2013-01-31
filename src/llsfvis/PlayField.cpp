/*
 * PlayField.cpp
 *
 *  Created on: 28.01.2013
 *      Author: daniel
 */

#include "PlayField.h"
#include <iostream>

namespace LLSFVis {

PlayField::PlayField() {
	set_size_request(800, 800);

}

void PlayField::add_machine(Machine* machine) {
	_machines.push_back(machine);
}

void PlayField::add_puck(Puck* puck) {
	_pucks.push_back(puck);
}

bool PlayField::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	//Blackground white
	cr->set_source_rgb(1, 1, 1);
	cr->paint();

	//draw machines
	for (std::vector<Machine*>::iterator iter_machines = _machines.begin();
			iter_machines != _machines.end(); ++iter_machines) {
		draw_machine(cr, **iter_machines);
	}

	//draw puck
	for (std::vector<Puck*>::iterator iter_pucks = _pucks.begin();
			iter_pucks != _pucks.end(); ++iter_pucks) {
		draw_puck(cr, **iter_pucks);
	}

	cr->restore();
	return true;
}

void PlayField::draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
		const Machine& machine) {
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(2.0); //todo move to conffile

	int leftX = machine.getPosX() - MACHINESIZE / 2;
	int rightX = machine.getPosX() + MACHINESIZE / 2;
	int upperY = machine.getPosY() - MACHINESIZE / 2;
	int lowerY = machine.getPosY() + MACHINESIZE / 2;

	cr->move_to(leftX, upperY);
	cr->line_to(rightX, upperY);
	cr->line_to(rightX, lowerY);
	cr->line_to(leftX, lowerY);
	cr->line_to(leftX, upperY);

	draw_machine_t(cr, MACHINESIZE / 3, machine.getPosX(), machine.getPosY(),
			machine.getOrientation());
	draw_text(cr, machine.getPosX() + MACHINESIZE / 2 + 10,
			machine.getPosY() + 10, machine.getTextDescription());
	cr->stroke();

	cr->restore();
}

void PlayField::draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr,
		int size, int x, int y, Machine::Orientation orientation) {
	switch (orientation) {
	case Machine::NORTH:

		cr->move_to(x - size / 2, y - size / 2);
		cr->line_to(x + size / 2, y - size / 2);

		cr->move_to(x, y - size / 2);
		cr->line_to(x, y + size / 2);
		break;
	case Machine::SOUTH:
		cr->move_to(x - size / 2, y + size / 2);
		cr->line_to(x + size / 2, y + size / 2);
		cr->move_to(x, y - size / 2);
		cr->line_to(x, y + size / 2);
		break;

	case Machine::WEST:
		cr->move_to(x - size / 2, y - size / 2);
		cr->line_to(x - size / 2, y + size / 2);
		cr->move_to(x - size / 2, y);
		cr->line_to(x + size / 2, y);
		break;
	case Machine::EAST:
		cr->move_to(x + size / 2, y - size / 2);
		cr->line_to(x + size / 2, y + size / 2);
		cr->move_to(x - size / 2, y);
		cr->line_to(x + size / 2, y);
		break;
	default:
		break;
	}
}

void PlayField::draw_puck(const Cairo::RefPtr<Cairo::Context>& cr,
		const Puck& puck) {
	cr->save();
	cr->set_line_width(4);
	cr->arc(puck.getPosX(), puck.getPosY(), PUCKSIZE / 3, 0.0, 2.0 * M_PI);
	cr->set_source_rgba(0.5, 0, 0, 0.6);
	cr->fill_preserve();
	cr->set_source_rgb(1, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayField::draw_text(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y,
		std::string text) {
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	Pango::FontDescription font;

	font.set_family("Monospace");
	font.set_weight(Pango::WEIGHT_NORMAL);
	font.set_size(Pango::SCALE * 8);

	Glib::RefPtr<Pango::Layout> layout = create_pango_layout(text);
	layout->set_font_description(font);

	//int textWidth, textHeight;
	//layout->get_pixel_size(textWidth, textHeight);

	cr->move_to(x, y);
	layout->show_in_cairo_context(cr);
	cr->restore();
}

PlayField::~PlayField() {
	// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
