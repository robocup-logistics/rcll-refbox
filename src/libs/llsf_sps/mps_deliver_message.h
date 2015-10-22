#include "mps_message.h"

class MPSDeliverSideMessage : public MPSMessage {
 private:
  int side;

 public:
  MPSDeliverSideMessage();
  MPSDeliverSideMessage(int side);
  int getSide();
  void setSide(int side);
};
