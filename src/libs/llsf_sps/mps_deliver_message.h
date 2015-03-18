class MPSDeliverSideMessage {
 private:
  int side;

 public:
  MPSDeliverSideMessage();
  MPSDeliverSideMessage(int side);
  int setSide();
  void getSide(int side);
};
