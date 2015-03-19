#ifndef MPSLIST_H
#define MPSLIST_H

#include <vector>
#include "mps_info.h"

class MPSList {
 private:
  static MPSList* instance;
  MPSList();

 public:
  std::vector<MPSInfo*> hosts;
  static MPSList* getInstance();
}; 


#endif // MPSLIST_H
