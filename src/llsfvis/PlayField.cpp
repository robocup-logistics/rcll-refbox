/***************************************************************************
 *  Playfield.cpp - Widget to represent the LLSF Area
 *
 *  Created: Mon Jan 31 17:46:35 2013
 *  Copyright  2013  Daniel Ewert (daniel.ewert@ima.rwth-aachen.de)
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PlayField.h"
#include <iostream>

namespace LLSFVis {

PlayField::PlayField() {

}

void PlayField::add_machine(Machine* machine) {
	machines_.push_back(machine);
}

void PlayField::add_puck(Puck* puck) {
	pucks_.push_back(puck);
}

bool PlayField::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->scale(get_allocated_height() / FIELDSIZE,
			get_allocated_width() / FIELDSIZE);

	//Blackground white
	cr->set_source_rgb(1, 1, 1);
	cr->paint();

	draw_delivery_zone(cr);
	draw_starting_zone(cr);

	//draw machines
	for (std::list<const Machine*>::iterator iter_machines = machines_.begin();
			iter_machines != machines_.end(); ++iter_machines) {
		draw_machine(cr, **iter_machines);
	}

	//draw puck
	for (std::list<const Puck*>::iterator iter_pucks = pucks_.begin();
			iter_pucks != pucks_.end(); ++iter_pucks) {
		draw_puck(cr, **iter_pucks);
	}

	cr->restore();
	return true;
}

void PlayField::draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
		const Machine& machine) {
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(0.01); //todo move to conffile

	double leftX = machine.getPosX() - MACHINESIZE / 2;
	double rightX = machine.getPosX() + MACHINESIZE / 2;
	double upperY = machine.getPosY() - MACHINESIZE / 2;
	double lowerY = machine.getPosY() + MACHINESIZE / 2;

	cr->move_to(leftX, upperY);
	cr->line_to(rightX, upperY);
	cr->line_to(rightX, lowerY);
	cr->line_to(leftX, lowerY);
	cr->line_to(leftX, upperY);

	draw_machine_t(cr, MACHINESIZE / 3, machine.getPosX(), machine.getPosY(),
			machine.getOrientation());
	draw_text(cr, machine.getPosX() + MACHINESIZE / 2 + 0.10,
			machine.getPosY() + 0.10, machine.getTextDescription());
	cr->stroke();

	cr->restore();
}

void PlayField::draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr,
		double size, double x, double y, Machine::Orientation orientation) {
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
	cr->set_line_width(0.04);
	cr->arc(puck.getPosX(), puck.getPosY(), PUCKSIZE, 0.0, 2.0 * M_PI);
	cr->set_source_rgba(0.5, 0, 0, 0.6);
	cr->fill_preserve();
	cr->set_source_rgb(1.0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayField::draw_delivery_zone(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->set_line_width(0.01);
	cr->move_to(0, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(ZONEHEIGHT, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(ZONEHEIGHT, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(0, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(0, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->set_source_rgb(0.0, 1.0, 0.0);
	cr->fill_preserve();
	cr->set_source_rgb(0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayField::draw_starting_zone(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->set_line_width(0.01);
	cr->move_to(FIELDSIZE - ZONEHEIGHT, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE - ZONEHEIGHT, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE - ZONEHEIGHT, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->set_source_rgb(0.0, 0.0, 1.0);
	cr->fill_preserve();
	cr->set_source_rgb(0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayField::draw_text(const Cairo::RefPtr<Cairo::Context>& cr, double x,
		double y, std::string text) {
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	Pango::FontDescription font;

	font.set_family("Monospace");
	font.set_stretch(Pango::STRETCH_CONDENSED);
	font.set_weight(Pango::WEIGHT_NORMAL);
	font.set_size(Pango::SCALE * 0.06);

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
