#include <mps_pick_place_1_message.h>

MPSPickPlace1ProduceEndMessage::MPSPickPlace1ProduceEndMessage() {
  this->updown = 0;
}

MPSPickPlace1ProduceEndMessage::MPSPickPlace1ProduceEndMessage(int updown) {
  this->updown = updown;
}

int MPSPickPlace1ProduceEndMessage::getUpdown() {
  return this->updown;
}

void MPSPickPlace1ProduceEndMessage::setUpdown(int updown) {
  this->updown;
}
