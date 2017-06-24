#include "logger.h"

#include <iomanip>
#include <sstream>

const logger& logger::get() {
    static logger the_logger;
    return the_logger;
}

std::string logger::pretty_time() const {
    using namespace std::chrono;
    std::stringstream ss;
    auto time = high_resolution_clock::now() - start;
    auto h = duration_cast<hours>(time);
    auto m = duration_cast<minutes>(time -= h);
    auto s = duration_cast<seconds>(time -= m);
    auto ms = duration_cast<seconds>(time -= s);
    ss << std::setw(3) << std::setfill('0') << h.count() << ":"
       << std::setw(2) << m.count() << ":"
       << std::setw(2) << s.count() << ":"
       << std::setw(3) << ms.count();
    return ss.str();
}

logger::logger() {
    start = std::chrono::high_resolution_clock::now();
}