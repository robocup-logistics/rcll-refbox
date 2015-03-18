#include <mps_pick_place_2_message.h>

MPSPickPlace2ProduceRingMessage::MPSPickPlace2ProduceRingMessage() {
  this->ring = 0;
}

MPSPickPlace2ProduceRingMessage::MPSPickPlace2ProduceRingMessage(int ring) {
  this->ring = ring;
}

int MPSPickPlace2ProduceRingMessage::getRing() {
  return this->ring;
}

void MPSPickPlace2ProduceRingMessage::setRing(int ring) {
  this->ring = ring;
}
