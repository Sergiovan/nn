#pragma once

#ifndef __WIN32__
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
#include <shlwapi.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x04
#endif
#endif

#include <chrono>
#include <utility>
#include <cstdio>
#include <iostream>

#include "os.h"

class logger {
public:
    template <typename stream, typename... Args>
    void print(stream&& s, Args&&... args) const {
        if(true) { //TODO Change
            s << "[" << pretty_time() << "] ";
        }
        (print_single(s, args), ...);
        s << std::endl;
    }
    
    template <typename stream, typename... Args>
    void print_raw(stream&& s, Args&&... args) const {
        (s << ... << std::forward<Args>(args));
    }
    
    template <typename stream, typename Arg>
    void print_single(stream&& s, Arg&& arg) const {
        s << std::forward<Arg>(arg) << " ";
    }
    
    std::string pretty_time() const;
    
    static const logger& get();
private:
    logger();
    std::chrono::high_resolution_clock::time_point start;
};

namespace Color {
    constexpr const char* reset  = "\033[0m";
    constexpr const char* red    = "\033[31m";
    constexpr const char* yellow = "\033[33m";
    constexpr const char* cyan   = "\033[36m";
}

namespace Logger {
    
    template <typename stream, typename... Args>
    static void print(stream&& s, Args&&... args) {
        logger::get().print(std::forward<stream>(s), std::forward<Args>(args)...);
    }
    
    template <typename... Args>
    static void info(Args&&... args) {
        logger::get().print(std::cout, "\033[36m[INFO ]\033[0m", args...);
    }
    
    template <typename... Args>
    static void warning(Args&&... args) {
        logger::get().print(std::cout, "\033[33m[WARN ]\033[0m", args...);
    }
    
    template <typename... Args>
    static void error(Args&&... args) {
        logger::get().print(std::cout, "\033[31m[ERROR]\033[0m", args...);
    }
    
    template <typename... Args>
    static void debug(Args&&... args) {
#ifdef DEBUG
        logger::get().print(std::cout, "[DEBUG]", args...);
#endif
    }
    
}