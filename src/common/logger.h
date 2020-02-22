#pragma once

#include <sstream>
#include <mutex>
#include <chrono>

#include "common/defs.h"

class logger_instance;

class _logger {
public:
    _logger();
    
    template <typename T>
    T create_instance() {
        return T{*this};
    }
    
    void print(const logger_instance& instance);
private:
    std::chrono::steady_clock::time_point start;
    std::mutex print_mutex{};
};

class logger_instance {
public:
    
    logger_instance(_logger& logger);
    
    ~logger_instance();
    logger_instance(const logger_instance& o) = delete;
    logger_instance(logger_instance&& o);
    logger_instance& operator=(const logger_instance& o) = delete;
    logger_instance& operator=(logger_instance&& o) = delete;
    
    
    template <typename T>
    logger_instance& operator<<(T t) {
        ss << t;
        return *this;
    }
protected:
    std::stringstream ss{};
private:
    _logger& logger;
    
    bool done{false};
    
    friend class _logger;
};

namespace logger {    
    alignas(_logger)
    extern _logger default_logger;
    
    logger_instance error();
    logger_instance warn();
    logger_instance info();
    logger_instance debug();
}
