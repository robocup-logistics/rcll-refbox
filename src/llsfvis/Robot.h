/***************************************************************************
 *  Robot.h - Data class representing a robot
 *
 *  Created: Mon Feb 11 12:55:35 2013
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

#ifndef ROBOT_H_
#define ROBOT_H_
namespace LLSFVis {
class Robot {
public:
	Robot(std::string id) :
			id_(id), posX_(0), posY_(0), orientation_(0), last_heard(0) {
	}
	;

	const std::string& getId() const {
		return id_;
	}

	time_t getLastHeard() const {
		return last_heard;
	}

	void setLastHeard(time_t lastHeard) {
		last_heard = lastHeard;
	}

	double getOrientation() const {
		return orientation_;
	}

	void setOrientation(double orientation) {
		orientation_ = orientation;
	}

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

	void setPose(double posX, double posY, double orientation) {
		posX_ = posX;
		posY_ = posY;
		orientation_ = orientation;
	}

private:
	std::string id_;
	double posX_;
	double posY_;
	double orientation_;
	time_t last_heard;
};

}/* namespace LLSFVis */
#endif /* ROBOT_H_ */
