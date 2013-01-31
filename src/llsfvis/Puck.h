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
			_posX(posX), _posY(posY), _uid(uid), _state(state) {

	}
	;

	~Puck() {

	}
	;

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

	const std::string& getState() const {
		return _state;
	}

	void setState(const std::string& state) {
		_state = state;
	}

	const std::string& getUid() const {
		return _uid;
	}

	void setUid(const std::string& uid) {
		_uid = uid;
	}

private:
	int _posX;
	int _posY;
	std::string _uid;
	std::string _state;
};

}/* namespace LLSFVis */
#endif /* PUCK_H_ */
