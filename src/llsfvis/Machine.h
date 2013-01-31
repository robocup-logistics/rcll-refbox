/*
 * Machine.h
 *
 *  Created on: 29.01.2013
 *      Author: daniel
 */

#ifndef MACHINE_H_
#define MACHINE_H_

namespace LLSFVis {

class Machine {
public:

	enum Orientation {
		NORTH, WEST, EAST, SOUTH
	};

	Machine(int posX, int posY, Orientation ori, std::string textDescription) :
			_posX(posX), _posY(posY), _orientation(ori), _textDescription(
					textDescription) {
	}
	;

	virtual ~Machine() {
	}
	;

	Orientation getOrientation() const {
		return _orientation;
	}

	void setOrientation(Orientation orientation) {
		_orientation = orientation;
	}

	int getPosX() const {
		return _posX;
	}

	void setPosX(int posX) {
		_posX = posX;
	}

	int getPosY() const {
		return _posY;
	}

	void setPosY(int posY) {
		_posY = posY;
	}

	const std::string& getTextDescription() const {
		return _textDescription;
	}

	void setTextDescription(const std::string& textDescription) {
		_textDescription = textDescription;
	}

private:
	int _posX;
	int _posY;
	Orientation _orientation;
	std::string _textDescription;
};

} /* namespace LLSFVis */
#endif /* MACHINE_H_ */
