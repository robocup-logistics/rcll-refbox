#include <mps_list.h>

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

MPSInfo* MPSList::getHostFromSocket(int descriptor) {
  for(int i = 0; i < this->hosts.size(); i++) {
    if(hosts.at(i)->getSocket() == descriptor) {
      return hosts.at(i);
    }
  }
  std::cout << "Device not found" << std::endl; // Here we need a exception
}
