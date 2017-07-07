// An exception for timeouts.
//
#pragma once

#include <stdexcept>
#include <string>

namespace llsfrb {
#if 0
}
#endif
namespace modbus {
#if 0
}
#endif

class timeoutException : public std::runtime_error {
  public:
  timeoutException(const std::string& msg) : std::runtime_error(msg) {};
};

}
}
