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

#ifndef PLAYFIELD_H_
#define PLAYFIELD_H_

#include <gtkmm.h>
#include <list>
#include "Machine.h"
#include "Puck.h"

namespace LLSFVis {


/**
 * Widget to display the playfield. Takes
 */
class PlayField: public Gtk::DrawingArea {
public:
	PlayField();
	virtual ~PlayField();
	void add_machine(Machine* machine);
	void add_puck(Puck* puck);

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
private:
	static double scale(const double val);
	void draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
			const Machine& machine);
	void draw_puck(const Cairo::RefPtr<Cairo::Context>& cr, const Puck& puck);
	void draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr, double size,
			double wPos, double hPos, Machine::Orientation orientation);
	void draw_text(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y,
			std::string text);
	void draw_delivery_zone(const Cairo::RefPtr<Cairo::Context>& cr);
	void draw_starting_zone(const Cairo::RefPtr<Cairo::Context>& cr);

	std::list<const Machine*> _machines;
	std::list<const Puck*> _pucks;

	static const double MACHINESIZE = 0.50;
	static const double PUCKSIZE = 0.08;
	static const double FIELDSIZE = 5.6;
	static const double ZONEHEIGHT = 0.4;
	static const double ZONEWIDTH = 1.2;
};

} /* namespace LLSFVis */
#endif /* PLAYFIELD_H_ */
