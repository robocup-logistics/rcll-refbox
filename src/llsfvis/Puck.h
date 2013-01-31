#ifndef PUCK_H_
#define PUCK_H_
namespace LLSFVis {

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
