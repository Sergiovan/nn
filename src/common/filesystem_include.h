#pragma once

#ifdef __has_include
#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace std::filesystem {
    using namespace std::experimental::filesystem;
}
#else
#error "filesystem library does not exist!"
#endif
#endif