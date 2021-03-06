#include "common/logger.h"

#include <iostream>
#include <iomanip>

using namespace logger;

alignas(_logger)
_logger logger::default_logger{};

_logger::_logger() 
    : start{std::chrono::steady_clock::now()} { 
    
}

void _logger::print(const logger_instance& instance) {
    if (instance.done) return;
    
    using namespace std::chrono;
    
    std::lock_guard grd{print_mutex};
    
    std::cout << "[" << std::setfill('0') << std::setw(10) 
              << duration_cast<microseconds>(steady_clock::now() - start).count()
              << "µs]" << " " << instance.ss.str() << "\n";
    std::cout.flush();
}

logger_instance::logger_instance(_logger& logger) : logger{logger} {}

logger_instance::~logger_instance() {
    logger.print(*this);
}

logger_instance::logger_instance(logger_instance && o) 
    : logger{o.logger} {
    o.done = true;    
    std::swap(ss, o.ss);
}

dummy_instance::dummy_instance(_logger&) { }

class error_instance : public logger_instance {
public:
    error_instance(_logger& logger) : logger_instance {logger} {
        ss << "\u001b[31;1m[ERROR]\u001b[0m ";
    }
};

class warn_instance : public logger_instance {
public:
    warn_instance(_logger& logger) : logger_instance {logger} {
        ss << "\u001b[38;5;202m[WARN ]\u001b[0m ";
    }
};

class info_instance : public logger_instance {
public:
    info_instance(_logger& logger) : logger_instance {logger} {
        ss << "\u001b[36m[INFO ]\u001b[0m ";
    }
};

class debug_instance : public logger_instance {
public:
    debug_instance(_logger& logger) : logger_instance {logger} {
        ss << "\u001b[37;1m[DEBUG]\u001b[0m ";
    }
};

logger_instance logger::error() {
    return error_instance{default_logger};
}

logger_instance logger::warn() {
    return warn_instance{default_logger};
}

logger_instance logger::info() {
    return info_instance{default_logger};
}

logger::debug_instance logger::debug() {
    return std::conditional_t<::debug, ::debug_instance, dummy_instance>{default_logger};
}
