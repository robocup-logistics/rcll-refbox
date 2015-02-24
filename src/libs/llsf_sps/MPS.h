#ifndef MPS_H
#define MPS_H

class MPS {
 public:
  virtual ~MPS() = 0;
  virtual void receiveData() = 0;
};

#endif // MPS_H
