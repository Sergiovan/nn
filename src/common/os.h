#pragma once

enum class OS {
    WINDOWS, LINUX, OTHER
};

#ifdef __WIN32__
static constexpr OS PLATFORM = OS::WINDOWS;
#elif __linux__
static constexpr PLATFORM = OS::LINUX;
#else
static constexpr PLATFORM = OS::OTHER;
#endif