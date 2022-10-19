#ifndef INPUT_INPUT_HH_
#define INPUT_INPUT_HH_

#include <iostream>

namespace input {

std::string read(const std::string& description);

} /// namespace input

namespace input {

std::string read(const std::string& description) {
  std::string ret;

  std::cout << description;
  std::cout.flush();
  std::cin >> ret;

  return ret;
}

} /// namespace input

#endif /// INPUT_INPUT_HH_
