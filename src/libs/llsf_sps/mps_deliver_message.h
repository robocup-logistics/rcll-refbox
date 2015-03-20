class MPSDeliverSideMessage {
 private:
  int side;

 public:
  MPSDeliverSideMessage();
  MPSDeliverSideMessage(int side);
  int getSide();
  void setSide(int side);
};
