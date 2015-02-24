#ifndef MPSLIST_H
#define MPSLIST_H

#include <vector>
#include <MPSInfo.h>

class MPSList {
 private:
  static MPSList* instance;
  MPSList();

 public:
  std::vector<MPSInfo*> hosts;
  static MPSList* getInstance();
  MPSInfo* getHostFromSocket(int descriptor);
}; 


#endif // MPSLIST_H
