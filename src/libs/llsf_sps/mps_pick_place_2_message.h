#include "mps_message.h"

class MPSPickPlace2ProduceRingMessage : public MPSMessage {
 private:
  int ring;

 public:
  MPSPickPlace2ProduceRingMessage();
  MPSPickPlace2ProduceRingMessage(int ring);
  int getRing();
  void setRing(int ring);
};
