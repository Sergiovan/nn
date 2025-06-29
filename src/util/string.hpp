#pragma once

#include <string>

namespace util {

constexpr const char* BOLD_RED = "\033[1;31m";
constexpr const char* BOLD_WHITE = "\033[1;37m";
constexpr const char* RESET_COLOR = "\033[0m";

/* Returns `original` with all instances of `substr` replaced with `replacement` */
std::string replace(const std::string& original, const std::string& substr,
                    const std::string& replacement);

} // namespace util