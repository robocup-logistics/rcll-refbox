#include "mps_message.h"

class MPSPickPlace1ProduceEndMessage : public MPSMessage {
 private:
  int updown;

 public:
  MPSPickPlace1ProduceEndMessage();
  MPSPickPlace1ProduceEndMessage(int updown);
  int getUpdown();
  void setUpdown(int updown);
};
