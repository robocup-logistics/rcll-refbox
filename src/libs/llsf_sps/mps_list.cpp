#include "mps_list.h"

#include <iostream>

MPSList* MPSList::instance = 0;

MPSList::MPSList() {
}

MPSList* MPSList::getInstance() {
  if(instance == 0) {
    instance = new MPSList();
  }

  return instance;
}
