// An exception for timeouts.
//
#pragma once

#include <stdexcept>
#include <string>

class timeoutException : public std::runtime_error {
  public:
  timeoutException(const std::string& msg) : std::runtime_error(msg) {};
};
