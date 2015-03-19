#include "mps_incoming_station_message.h"

MPSIncomingStationGiveCapMessage::MPSIncomingStationGiveCapMessage() {
  this->side = 0;
  this->color = 0;
}

MPSIncomingStationGiveCapMessage::MPSIncomingStationGiveCapMessage(int color, int side) {
  this->side = side;
  this->color = color;
}

int MPSIncomingStationGiveCapMessage::getSide() {
  return this->side;
}

int MPSIncomingStationGiveCapMessage::getColor() {
  return this->color;
}

void MPSIncomingStationGiveCapMessage::setSide(int side) {
  this->side = side;
}

void MPSIncomingStationGiveCapMessage::setColor(int color) {
  this->color = color;
}
