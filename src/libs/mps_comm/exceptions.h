// An exception for timeouts.
//
#pragma once

#include <stdexcept>
#include <string>

namespace llsfrb {
#if 0
}
#endif
namespace mps_comm {
#if 0
}
#endif

class timeout_exception : public std::runtime_error {
  public:
  timeout_exception(const std::string& msg) : std::runtime_error(msg) {};
};

}
}
