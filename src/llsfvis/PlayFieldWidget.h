/***************************************************************************
 *  Playfield.h - Widget to represent the LLSF Area
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

#ifndef PLAYFIELDWIDGET_H_
#define PLAYFIELDWIDGET_H_

#include <gtkmm.h>
#include <list>
#include "msgs/RobotInfo.pb.h"
#include "msgs/MachineInfo.pb.h"
#include "msgs/MachineCommands.pb.h"

namespace LLSFVis {

/**
 * Widget to display the playfield.^
 */
class PlayFieldWidget: public Gtk::DrawingArea {
public:
	PlayFieldWidget();
	virtual ~PlayFieldWidget();
	void update_machines(const llsf_msgs::MachineInfo& mSpecs);
	void update_robot_info(const llsf_msgs::RobotInfo& robotInfo);
	void update_pucks(const llsf_msgs::PuckInfo& pucks);
	sigc::signal<void,llsf_msgs::RemovePuckFromMachine&> signal_remove_puck();
protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
private:
	void create_context_menu(const llsf_msgs::Machine& machine, GdkEventButton* event);
	void draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
			const llsf_msgs::Machine& machine);
	void draw_puck(const Cairo::RefPtr<Cairo::Context>& cr, const llsf_msgs::Puck& puck);
	void draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr, double size,
			double wPos, double hPos, double orientation);
	void draw_machine_signal(
			const Cairo::RefPtr<Cairo::Context>& cr,
			const llsf_msgs::Machine& machine,
			const llsf_msgs::LightState& redState,
			const llsf_msgs::LightState& yellowState,
			const llsf_msgs::LightState& greenState);
	void draw_machine_light(const Cairo::RefPtr<Cairo::Context>& cr,
			double size, double x, double y, double r, double g, double b, bool blink);
	void draw_text(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y,
		       std::string text, bool centered = false);

	void draw_field_border(const Cairo::RefPtr<Cairo::Context>& cr);
	void draw_delivery_zone(const Cairo::RefPtr<Cairo::Context>& cr);
	void draw_starting_zone(const Cairo::RefPtr<Cairo::Context>& cr);
	void draw_robot(const Cairo::RefPtr<Cairo::Context>& cr,
			const llsf_msgs::Robot& bot);


	virtual bool on_clicked(GdkEventButton* event);

	virtual void on_contextmenu_clicked(const llsf_msgs::Puck& puck);

	const llsf_msgs::Machine* get_clicked_machine(gdouble x, gdouble y);

	Gtk::Menu* menu;
	Glib::RefPtr<Gtk::UIManager> uIManager_;
	Glib::RefPtr<Gtk::ActionGroup> actionGroup_;

	sigc::signal<void,llsf_msgs::RemovePuckFromMachine&> signal_remove_puck_;


	llsf_msgs::MachineInfo machines_;
	bool have_machine_info_;
	llsf_msgs::PuckInfo pucks_;
	bool have_puck_info_;
	llsf_msgs::RobotInfo robots_;
	bool have_robot_info_;
	const llsf_msgs::Machine* clicked_machine_;

	static constexpr double FIELDLINESSIZE = 0.02;
	static constexpr double BOTSIZE = 0.35;
	static constexpr double MACHINESIZE = 0.40;
	static constexpr double PUCKSIZE = 0.15;
	static constexpr double MINIPUCKSIZE = 0.08;
	static constexpr double FIELDSIZE = 5.6;
	static constexpr double FIELDBORDERSIZE = 0.2;
	static constexpr double ZONEHEIGHT = 1.0;
	static constexpr double ZONEWIDTH = 0.4;
	static constexpr double LOAREAHEIGHT = 0.6;
	static constexpr double LOSLOTSIZE = 0.1;
	static constexpr double LOSLOTX = 0.2 - LOSLOTSIZE / 2;
	static constexpr double LOSLOTY = 3.6 - LOSLOTSIZE / 2;
	static constexpr double DIST_TOLERANCE = 0.2;
	static constexpr double ORI_TOLERANCE = 0.2;

};

} /* namespace LLSFVis */
#endif /* PLAYFIELD_H_ */
