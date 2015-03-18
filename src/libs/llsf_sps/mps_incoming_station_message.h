class MPSIncomingStationGiveCapMessage {
 private:
  int color;
  int side;

 pubilc:
  MPSIncomingStationGiveCapMessage();
  MPSIncomingStationGiveCapMessage(int color, int side);
  int getSide();
  int getColor();
  void setSide(int side);
  void setColor(int color);
};
