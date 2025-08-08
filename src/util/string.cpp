#include "util/string.hpp"

#include <sstream>

std::string util::replace(const std::string& original,
                          const std::string& substr,
                          const std::string& replacement) {
  std::string_view view{original};
  std::stringstream ss{};
  uint64_t start = 0;
  auto pos = original.find(substr);

  while (pos != std::string::npos) {
    ss << view.substr(start, pos - start);
    ss << replacement;
    start = pos + replacement.length();

    pos = original.find(substr, start);
  }

  ss << view.substr(start);

  return ss.str();
}