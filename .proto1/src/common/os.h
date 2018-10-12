#pragma once

enum class OS {
    WINDOWS, LINUX, OTHER
};

#ifdef CMAKE
#undef __WIN32__
#define __linux__ 1
#endif

#ifdef __WIN32__
static constexpr OS PLATFORM = OS::WINDOWS;
#elif __linux__
static constexpr OS PLATFORM = OS::LINUX;
#else
static constexpr OS PLATFORM = OS::OTHER;
#endif