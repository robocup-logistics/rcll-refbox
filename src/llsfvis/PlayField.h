/*
 * PlayField.h
 *
 *  Created on: 28.01.2013
 *      Author: daniel
 */

#ifndef PLAYFIELD_H_
#define PLAYFIELD_H_

#include <gtkmm.h>
#include <vector>
#include "Machine.h"
#include "Puck.h"

namespace LLSFVis {

class PlayField: public Gtk::DrawingArea {
public:
	PlayField();
	virtual ~PlayField();
	void add_machine(Machine* machine);
	void add_puck(Puck* puck);

protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
private:
	void draw_machine(const Cairo::RefPtr<Cairo::Context>& cr,
			const Machine& machine);
	void draw_puck(const Cairo::RefPtr<Cairo::Context>& cr, const Puck& puck);
	void draw_machine_t(const Cairo::RefPtr<Cairo::Context>& cr, int size,
			int wPos, int hPos, Machine::Orientation orientation);
	void draw_text(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y,
			std::string text);

	std::vector<Machine*> _machines;
	std::vector<Puck*> _pucks;

	static const int MACHINESIZE = 50;
	static const int PUCKSIZE = 20;
};

} /* namespace LLSFVis */
#endif /* PLAYFIELD_H_ */
