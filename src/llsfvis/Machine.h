/***************************************************************************
 *  Machine.h - Data class representing a machine
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

#ifndef MACHINE_H_
#define MACHINE_H_

namespace LLSFVis {

/**
 * Data container for a machine
 */
class Machine {
public:

	typedef struct {
		bool red_on;
		bool yellow_on;
		bool green_on;
		bool yellow_blinking;
	} SignalState;

	Machine(double posX, double posY, double ori, std::string name,
			std::string type) :
			posX_(posX), posY_(posY), orientation_(ori), name_(name), type_(
					type) {
	}
	;

	virtual ~Machine() {
	}
	;

	double getOrientation() const {
		return orientation_;
	}

	void setOrientation(double orientation) {
		orientation_ = orientation;
	}

	double getPosX() const {
		return posX_;
	}

	void setPosX(int posX) {
		posX_ = posX;
	}

	double getPosY() const {
		return posY_;
	}

	void setPosY(int posY) {
		posY_ = posY;
	}

	const std::string& getName() const {
		return name_;
	}

	void setName(const std::string& name) {
		name_ = name;
	}

	const std::string& getType() const {
		return type_;
	}

	void setType(const std::string& type) {
		type_ = type;
	}

	const SignalState& getState() const {
		return state_;
	}

	void setState(const SignalState& state) {
		state_ = state;
	}

private:
	double posX_;
	double posY_;
	double orientation_;
	std::string name_;
	std::string type_;
	SignalState state_;
};

} /* namespace LLSFVis */
#endif /* MACHINE_H_ */
