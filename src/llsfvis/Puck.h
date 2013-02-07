/***************************************************************************
 *  Puck.h - Data class representing a puck
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

#ifndef PUCK_H_
#define PUCK_H_
namespace LLSFVis {

/**
 * Data container for a puck
 */
class Puck {
public:

	Puck(int posX, int posY, std::string uid, std::string state) :
			posX_(posX), posY_(posY), uid_(uid), state_(state) {

	}
	;

	~Puck() {

	}
	;

	double getPosX() const {
		return posX_;
	}

	void setPosX(double posX) {
		posX_ = posX;
	}

	double getPosY() const {
		return posY_;
	}

	void setPosY(double posY) {
		posY_ = posY;
	}

	const std::string& getState() const {
		return state_;
	}

	void setState(const std::string& state) {
		state_ = state;
	}

	const std::string& getUid() const {
		return uid_;
	}

	void setUid(const std::string& uid) {
		uid_ = uid;
	}

private:
	double posX_;
	double posY_;
	std::string uid_;
	std::string state_;
};

}/* namespace LLSFVis */
#endif /* PUCK_H_ */
