#include <mps_deliver_message.h>

MPSDeliverSideMessage::MPSDeliverSideMessage() {
  this->side = 0;
}

MPSDeliverSideMessage::MPSDeliverSideMessage(int side) {
  this->side = side;
}

int MPSDeliverSideMessage::setSide() {
  return this->side;
}

void MPSDeliverSideMessage::getSide(int side) {
  this->side = side;
}
