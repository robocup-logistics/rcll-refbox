#include "mps_message.h"

class MPSIncomingStationGiveCapMessage : public MPSMessage {
 private:
  int color;
  int side;

 public:
  MPSIncomingStationGiveCapMessage();
  MPSIncomingStationGiveCapMessage(int color, int side);
  int getSide();
  int getColor();
  void setSide(int side);
  void setColor(int color);
};
