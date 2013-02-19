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

#include "PlayFieldWidget.h"
#include "PuckDialog.h"
#include <iostream>

namespace LLSFVis {

PlayFieldWidget::PlayFieldWidget() :
		popup_(0) {
	add_events(Gdk::BUTTON_PRESS_MASK);
	signal_button_press_event().connect(
			sigc::mem_fun(*this, &PlayFieldWidget::on_clicked));

	actionGroup_ = Gtk::ActionGroup::create();
	actionGroup_->add(Gtk::Action::create("ContextEntry1", "Entry1"),
			sigc::bind<Glib::ustring>(
					sigc::mem_fun(*this,
							&PlayFieldWidget::on_contextmenu_clicked),
					"Entry1"));

	actionGroup_->add(Gtk::Action::create("ContextEntry2", "Entry2"),
			sigc::bind<Glib::ustring>(
					sigc::mem_fun(*this,
							&PlayFieldWidget::on_contextmenu_clicked),
					"Entry2"));

	actionGroup_->add(Gtk::Action::create("ContextEntry3", "Entry3"),
			sigc::bind<Glib::ustring>(
					sigc::mem_fun(*this,
							&PlayFieldWidget::on_contextmenu_clicked),
					"Entry3"));

	uIManager_ = Gtk::UIManager::create();
	uIManager_->insert_action_group(actionGroup_);

	Glib::ustring ui_info = "<ui>"
			"  <popup name='PopupMenu'>"
			"    <menuitem action='ContextEntry1'/>"
			"    <menuitem action='ContextEntry2'/>"
			"    <menuitem action='ContextEntry3'/>"
			"  </popup>"
			"</ui>";

	try {
		uIManager_->add_ui_from_string(ui_info);
	} catch (const Glib::Error& ex) {
		std::cerr << "building menus failed: " << ex.what();
	}

	popup_ = dynamic_cast<Gtk::Menu*>(uIManager_->get_widget("/PopupMenu"));
	if (!popup_)
		g_warning("menu not found");

}

void PlayFieldWidget::add_machine(const Machine* machine) {
	machines_.push_back(machine);
}

void PlayFieldWidget::add_puck(const Puck* puck) {
	pucks_.push_back(puck);
}

void PlayFieldWidget::add_robot(const Robot* robot) {
	bots_.push_back(robot);
}

bool PlayFieldWidget::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->scale((get_allocated_height() / (FIELDSIZE + 2 * FIELDBORDERSIZE)),
			-(get_allocated_width() / (FIELDSIZE + 2 * FIELDBORDERSIZE)));
	cr->translate(FIELDBORDERSIZE,-1* (FIELDSIZE+FIELDBORDERSIZE));

	//Blackground white
	cr->set_source_rgb(1, 1, 1);
	cr->paint();

	draw_field_border(cr);
	draw_delivery_zone(cr);
	draw_starting_zone(cr);

	//draw machines
	for (std::list<const Machine*>::iterator iter_machines = machines_.begin();
			iter_machines != machines_.end(); ++iter_machines) {
		draw_machine(cr, **iter_machines);
	}

	//draw pucks
	for (std::list<const Puck*>::iterator iter_pucks = pucks_.begin();
			iter_pucks != pucks_.end(); ++iter_pucks) {
		draw_puck(cr, **iter_pucks);
	}

	//draw_robots
	for (std::list<const Robot*>::iterator iter_bots = bots_.begin();
			iter_bots != bots_.end(); ++iter_bots) {
		draw_robot(cr, **iter_bots);
	}

	cr->restore();
	return true;
}

void PlayFieldWidget::draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
		const Machine& machine) {
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(FIELDLINESSIZE);

	double leftX = machine.getPosX() - MACHINESIZE / 2;
	double rightX = machine.getPosX() + MACHINESIZE / 2;
	double upperY = machine.getPosY() + MACHINESIZE / 2;
	double lowerY = machine.getPosY() - MACHINESIZE / 2;

	cr->move_to(leftX, upperY);
	cr->line_to(rightX, upperY);
	cr->line_to(rightX, lowerY);
	cr->line_to(leftX, lowerY);
	cr->line_to(leftX, upperY);

	draw_machine_t(cr, MACHINESIZE / 3, machine.getPosX(), machine.getPosY(),
			machine.getOrientation());
	draw_text(cr, leftX + MACHINESIZE / 10, upperY - MACHINESIZE / 15,
			machine.getName());
	draw_text(cr, leftX + MACHINESIZE / 10, lowerY + 4 * MACHINESIZE / 15,
			machine.getType());
	cr->stroke();

	const Machine::SignalState& state = machine.getState();
	if (state.red_on) {
		draw_machine_light(cr, MACHINESIZE / 5, rightX - MACHINESIZE / 5,
				upperY, 1, 0, 0);
	} else {
		draw_machine_light(cr, MACHINESIZE / 5, rightX - MACHINESIZE / 5,
				upperY, 0.8, 0, 0);
	}
	if (state.yellow_on) {
		draw_machine_light(cr, MACHINESIZE / 5, rightX - MACHINESIZE / 5,
				upperY - MACHINESIZE / 5, 1, 1, 0);
	} else {
		draw_machine_light(cr, MACHINESIZE / 5, rightX - MACHINESIZE / 5,
				upperY - MACHINESIZE / 5, 0.9, 0.9, 0);
	}
	if (state.green_on) {
		draw_machine_light(cr, MACHINESIZE / 5, rightX - MACHINESIZE / 5,
				upperY - 2 * MACHINESIZE / 5, 0, 1, 0);
	} else {
		draw_machine_light(cr, MACHINESIZE / 5, rightX - MACHINESIZE / 5,
				upperY - 2 * MACHINESIZE / 5, 0, 0.8, 0);
	}

	cr->restore();
}

void PlayFieldWidget::draw_machine_light(
		const Cairo::RefPtr<Cairo::Context>& cr, double size, double x,
		double y, double r, double g, double b) {
	//adjustment, so light is in the box
	x -= FIELDLINESSIZE / 2;
	y -= FIELDLINESSIZE / 2;

	cr->save();
	cr->set_line_width(FIELDLINESSIZE);
	cr->move_to(x, y);
	cr->line_to(x + size, y);
	cr->line_to(x + size, y - size);
	cr->line_to(x, y - size);
	cr->line_to(x, y);
	cr->stroke_preserve();
	cr->set_source_rgb(r, g, b);
	cr->fill();
	cr->restore();
}

void PlayFieldWidget::draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr,
		double size, double x, double y, double orientation) {
	cr->save();
	cr->translate(x, y);
	cr->rotate(orientation);
	cr->move_to(-size / 2, -size / 2);
	cr->line_to(size / 2, -size / 2);
	cr->move_to(0, -size / 2);
	cr->line_to(0, size / 2);

	cr->restore();
}

void PlayFieldWidget::draw_robot(const Cairo::RefPtr<Cairo::Context>& cr,
		const Robot& bot) {
	cr->save();
	cr->translate(bot.getPosX(),
			bot.getPosY());
	cr->rotate(bot.getOrientation());
	cr->set_line_width(0.01);
	cr->arc(0, 0, BOTSIZE / 2, 0.0, 2.0 * M_PI);
	cr->move_to(BOTSIZE / 2, 0);
	cr->line_to(0, BOTSIZE / 2 * -1);
	cr->line_to(0, BOTSIZE / 2);
	cr->line_to(BOTSIZE / 2, 0);
	cr->set_source_rgb(0, 0, 0);
	cr->stroke();
	cr->restore();

}

void PlayFieldWidget::draw_puck(const Cairo::RefPtr<Cairo::Context>& cr,
		const Puck& puck) {
	cr->save();
	cr->translate(FIELDBORDERSIZE, FIELDBORDERSIZE);
	cr->set_line_width(0.04);
	cr->arc(puck.getPosX(), puck.getPosY(), PUCKSIZE, 0.0, 2.0 * M_PI);
	cr->set_source_rgba(0.5, 0, 0, 0.6);
	cr->fill_preserve();
	cr->set_source_rgb(1.0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayFieldWidget::draw_starting_zone(
		const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->set_line_width(FIELDLINESSIZE);
	cr->move_to(0, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(ZONEHEIGHT, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(ZONEHEIGHT, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(0, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(0, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->set_source_rgb(0.0, 0.0, 1.0);
	cr->fill_preserve();
	cr->set_source_rgb(0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayFieldWidget::draw_delivery_zone(
		const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->set_line_width(FIELDLINESSIZE);
	cr->move_to(FIELDSIZE - ZONEHEIGHT, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE - ZONEHEIGHT, FIELDSIZE / 2 + ZONEWIDTH / 2);
	cr->line_to(FIELDSIZE - ZONEHEIGHT, FIELDSIZE / 2 - ZONEWIDTH / 2);
	cr->set_source_rgb(0.0, 1.0, 0.0);
	cr->fill_preserve();
	cr->set_source_rgb(0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayFieldWidget::draw_text(const Cairo::RefPtr<Cairo::Context>& cr,
		double x, double y, std::string text) {
	cr->save();
	cr->scale(1,-1); //Temporary flip y-axis back, otherwise text is shown head down
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

	cr->move_to(x, y*-1); //cope with flipped y-axis
	layout->show_in_cairo_context(cr);
	cr->restore();
}

bool PlayFieldWidget::on_clicked(GdkEventButton* event) {
	clicked_machine_ = get_clicked_machine(event->x, event->y);
	if (clicked_machine_ != NULL) {
		if (event->button == 3) {
			popup_->popup(event->button, event->time);
		}
	}
	return true;
}

void PlayFieldWidget::on_contextmenu_clicked(Glib::ustring entry) {
	if (entry == "Entry1") {
		PuckDialog pd("Set Registered Puck");
		int result = pd.run();
		if (result == Gtk::RESPONSE_OK) {
			std::cout << pd.get_value() << std::endl;
			//TODO handle response
		}

	} else if (entry == "Entry2") {

	}
}

void PlayFieldWidget::set_playfield(PlayField& playField) {
	machines_ = playField.getMachines();
}

void PlayFieldWidget::draw_field_border(
		const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->set_line_width(FIELDLINESSIZE);
	cr->set_source_rgb(0, 0, 0);

	cr->move_to(0, 0);
	cr->line_to(0, FIELDSIZE);
	cr->line_to(FIELDSIZE, FIELDSIZE);
	cr->line_to(FIELDSIZE, 0);
	cr->line_to(0, 0);
	cr->stroke();
	cr->restore();
}

void PlayFieldWidget::update_game_state(GameState& gameState) {
	bots_ = gameState.getRobots();
	pucks_ = gameState.getPucks();
	machines_ = gameState.getMachines();
}

const Machine* PlayFieldWidget::get_clicked_machine(gdouble x, gdouble y) {
	gdouble scaled_x = x
			/ (get_allocated_width() / (FIELDSIZE + FIELDBORDERSIZE * 2))
			- FIELDBORDERSIZE;
	gdouble scaled_y = FIELDSIZE - (y
			/ (get_allocated_height() / (FIELDSIZE + FIELDBORDERSIZE * 2))
			- FIELDBORDERSIZE);
	for (std::list<const Machine*>::iterator it = machines_.begin();
			it != machines_.end(); ++it) {
		if (scaled_x >= (*it)->getPosX() - MACHINESIZE / 2
				&& scaled_x <= (*it)->getPosX() + MACHINESIZE / 2
				&& scaled_y >= (*it)->getPosY() - MACHINESIZE / 2
				&& scaled_y <= (*it)->getPosY() + MACHINESIZE / 2) {
			return (*it);
		}
	}
	return NULL;
}

PlayFieldWidget::~PlayFieldWidget() {
// TODO Auto-generated destructor stub
}

} /* namespace LLSFVis */
