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

void PlayFieldWidget::add_puck(const Puck* puck) {
	pucks_.push_back(puck);
}

bool PlayFieldWidget::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->scale((get_allocated_height() / (FIELDSIZE + 2 * FIELDBORDERSIZE)),
			-(get_allocated_width() / (FIELDSIZE + 2 * FIELDBORDERSIZE)));
	cr->translate(FIELDBORDERSIZE, -1 * (FIELDSIZE + FIELDBORDERSIZE));

	//Blackground white
	cr->set_source_rgb(1, 1, 1);
	cr->paint();

	draw_field_border(cr);
	draw_delivery_zone(cr);
	draw_starting_zone(cr);

	//draw machines
	if (machines_ != NULL) {
		for (int i = 0; i < machines_->machines_size(); ++i) {
			draw_machine(cr, machines_->machines(i));
		}
	}

	//draw pucks
	for (std::list<const Puck*>::iterator iter_pucks = pucks_.begin();
			iter_pucks != pucks_.end(); ++iter_pucks) {
		draw_puck(cr, **iter_pucks);
	}

	//draw_robots
	if (robotInfo_ != NULL) {
		for (int i = 0; i < robotInfo_->robots_size(); ++i) {
			draw_robot(cr, robotInfo_->robots(i));
		}
	}

	cr->restore();
	return true;
}

void PlayFieldWidget::draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
		const llsf_msgs::MachineSpec& machine) {
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(FIELDLINESSIZE);
	double leftX = machine.pose().x() - MACHINESIZE / 2;
	double upperY = machine.pose().y() + MACHINESIZE / 2;
	double lowerY = machine.pose().y() - MACHINESIZE / 2;

	cr->rectangle(leftX, lowerY, MACHINESIZE, MACHINESIZE);

	draw_machine_t(cr, MACHINESIZE / 4, machine.pose().x(), machine.pose().y(),
			machine.pose().ori());
	draw_text(cr, leftX + MACHINESIZE / 10, upperY - MACHINESIZE / 15,
			machine.name());
	draw_text(cr, leftX + MACHINESIZE / 10, lowerY + 4 * MACHINESIZE / 15,
			machine.type());
	cr->stroke();

	double puck_x = leftX + MACHINESIZE * 0.6;
	double puck_y = lowerY + MACHINESIZE * 0.15;
	cr->save();
	cr->set_source_rgb(0.9,0,0);
	for ( int i=0; i<machine.loaded_with_size();++i){
		cr->arc(puck_x + i* (MINIPUCKSIZE * 1.15),puck_y,MINIPUCKSIZE/2,0,2*M_PI);
		cr->fill();
	}
	cr->restore();

	llsf_msgs::LightState redstate = llsf_msgs::LightState::OFF, yellowstate =
			llsf_msgs::LightState::OFF, greenstate = llsf_msgs::LightState::OFF;

	for (int i = 0; i < machine.lights_size(); ++i) {
		const llsf_msgs::LightSpec& light = machine.lights(i);

		switch (light.color()) {
		case llsf_msgs::LightColor::RED:
			redstate = light.state();
			break;
		case llsf_msgs::LightColor::YELLOW:
			yellowstate = light.state();
			break;
		case llsf_msgs::LightColor::GREEN:
			greenstate = light.state();
			break;
		}
	}
	draw_machine_signal(cr, machine, redstate, yellowstate, greenstate);
	cr->restore();
}

void PlayFieldWidget::draw_machine_signal(
		const Cairo::RefPtr<Cairo::Context>& cr,
		const llsf_msgs::MachineSpec& machine,
		const llsf_msgs::LightState& redState,
		const llsf_msgs::LightState& yellowState,
		const llsf_msgs::LightState& greenState) {

	float r, g, b = 0.0;
	bool blink = false;
	switch (redState) {
	case llsf_msgs::LightState::OFF:
		r = 0.8;
		g = 0;
		b = 0;
		blink = false;
		break;
	case llsf_msgs::LightState::BLINK:
		r = 1;
		g = 0;
		b = 0;
		blink = true;
		break;
	case llsf_msgs::LightState::ON:
		r = 1;
		g = 0;
		b = 0;
		blink = false;
		break;
	}

	draw_machine_light(cr, MACHINESIZE / 5,
			machine.pose().x() + MACHINESIZE * 3 / 10,
			machine.pose().y() + MACHINESIZE * 3 / 10, r, g, b, blink);

	switch (yellowState) {
	case llsf_msgs::LightState::OFF:
		r = 0.8;
		g = 0.8;
		b = 0;
		blink = false;
		break;
	case llsf_msgs::LightState::BLINK:
		r = 1;
		g = 1;
		b = 0;
		blink = true;
		break;
	case llsf_msgs::LightState::ON:
		r = 1;
		g = 1;
		b = 0;
		blink = false;
		break;
	}

	draw_machine_light(cr, MACHINESIZE / 5,
			machine.pose().x() + MACHINESIZE * 3 / 10,
			machine.pose().y() + MACHINESIZE / 10, r, g, b, blink);

	switch (greenState) {
	case llsf_msgs::LightState::OFF:
		r = 0;
		g = 0.8;
		b = 0;
		blink = false;
		break;
	case llsf_msgs::LightState::BLINK:
		r = 0;
		g = 1;
		b = 0;
		blink = true;
		break;
	case llsf_msgs::LightState::ON:
		r = 0;
		g = 1;
		b = 0;
		blink = false;
		break;
	}
	draw_machine_light(cr, MACHINESIZE / 5,
			machine.pose().x() + MACHINESIZE * 3 / 10,
			machine.pose().y() - MACHINESIZE / 10, r, g, b, blink);
}

void PlayFieldWidget::draw_machine_light(
		const Cairo::RefPtr<Cairo::Context>& cr, double size, double x,
		double y, double r, double g, double b, bool blink) {
	//adjustment, so light is in the box
	x -= FIELDLINESSIZE / 2;
	y -= FIELDLINESSIZE / 2;

	cr->save();
	cr->set_line_width(FIELDLINESSIZE);
	cr->rectangle(x, y, size, size);
	cr->stroke_preserve();
	cr->set_source_rgb(r, g, b);
	cr->fill();
	if (blink) {
		cr->arc(x + size / 2, y + size / 2, size, 0, 2 * M_PI);
		cr->stroke();
	}
	cr->restore();
}

void PlayFieldWidget::draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr,
		double size, double x, double y, double orientation) {
	cr->save();
	cr->translate(x, y);
	cr->rotate(orientation);
	cr->move_to(-size / 2, 0);
	cr->line_to(size / 2, 0);
	cr->move_to(0, 0);
	cr->line_to(0, size / 2);

	cr->restore();
}

void PlayFieldWidget::draw_robot(const Cairo::RefPtr<Cairo::Context>& cr,
		const llsf_msgs::Robot& bot) {
	cr->save();
	cr->translate(bot.pose().x(), bot.pose().y());
	cr->rotate(bot.pose().ori());
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
	cr->arc(puck.getPosX(), puck.getPosY(), PUCKSIZE/2, 0.0, 2.0 * M_PI);
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
	cr->rectangle(0, FIELDSIZE / 2 - ZONEHEIGHT / 2, ZONEWIDTH, ZONEHEIGHT);
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
	cr->rectangle(FIELDSIZE - ZONEWIDTH, FIELDSIZE / 2 - ZONEHEIGHT / 2,
			ZONEWIDTH, ZONEHEIGHT);
	cr->set_source_rgb(0.0, 1.0, 0.0);
	cr->fill_preserve();
	cr->set_source_rgb(0, 0, 0);
	cr->stroke();
	cr->restore();
}

void PlayFieldWidget::draw_text(const Cairo::RefPtr<Cairo::Context>& cr,
		double x, double y, std::string text) {
	cr->save();
	cr->scale(1, -1); //Temporary flip y-axis back, otherwise text is shown head down
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

	cr->move_to(x, y * -1); //cope with flipped y-axis
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

void PlayFieldWidget::draw_field_border(
		const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->save();
	cr->set_line_width(FIELDLINESSIZE);
	cr->set_source_rgb(0, 0, 0);

	cr->rectangle(0, 0, FIELDSIZE, FIELDSIZE);
	//draw insertion area
	cr->move_to(ZONEWIDTH, 0);
	cr->line_to(ZONEWIDTH, FIELDSIZE);

	cr->move_to(FIELDSIZE - ZONEWIDTH, 0);
	cr->line_to(FIELDSIZE - ZONEWIDTH, FIELDSIZE);

	//draw late order slot
	cr->rectangle(LOSLOTX, LOSLOTY, LOSLOTSIZE, LOSLOTSIZE);
	cr->move_to(0, FIELDSIZE / 2 + ZONEHEIGHT / 2 + LOAREAHEIGHT);
	cr->line_to(ZONEWIDTH, FIELDSIZE / 2 + ZONEHEIGHT / 2 + LOAREAHEIGHT);

	cr->stroke();
	cr->restore();
}

void PlayFieldWidget::update_robot_info(llsf_msgs::RobotInfo& robotInfo) {
	robotInfo_ = &robotInfo;
}

void PlayFieldWidget::update_machines(llsf_msgs::MachineSpecs& mSpecs) {
	machines_ = &mSpecs;
}

const llsf_msgs::MachineSpec* PlayFieldWidget::get_clicked_machine(gdouble x,
		gdouble y) {
	gdouble scaled_x = x
			/ (get_allocated_width() / (FIELDSIZE + FIELDBORDERSIZE * 2))
			- FIELDBORDERSIZE;
	gdouble scaled_y = FIELDSIZE
			- (y / (get_allocated_height() / (FIELDSIZE + FIELDBORDERSIZE * 2))
					- FIELDBORDERSIZE);
	for (int i = 0; i < machines_->machines_size(); ++i) {
		float x = machines_->machines(i).pose().x();
		float y = machines_->machines(i).pose().y();
		if (scaled_x >= x - MACHINESIZE / 2 && scaled_x <= x + MACHINESIZE / 2
				&& scaled_y >= y - MACHINESIZE / 2
				&& scaled_y <= y + MACHINESIZE / 2) {
			return &(machines_->machines(i));
		}
	}
	return NULL;
}

PlayFieldWidget::~PlayFieldWidget() {
}

} /* namespace LLSFVis */
