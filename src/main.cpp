#include <print>

#include "driver/driver.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::println("Please provide a file to compile");
    return 1;
  }

  driver::Driver driver{argc, argv};

  return driver.run();
}