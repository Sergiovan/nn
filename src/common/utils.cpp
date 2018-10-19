#include "common/utils.h"
#include <iomanip>
#include <iostream>
#include <utility>

namespace logger {

LoggerMessage::LoggerMessage(Logger& logger,bool timestamp, const std::string& pre) 
 : logger(logger), timestamp(timestamp), pre(pre) 
 { }

LoggerMessage::~LoggerMessage() {
    std::lock_guard<std::mutex> guard{logger.get_lock()};
    if(timestamp) {
        put_timestamp();
    }
    std::cout << pre << ss.str() << color::reset << std::flush;
}

void LoggerMessage::put_timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto millis = now - time_point_cast<seconds>(now);
    std::time_t now_c = system_clock::to_time_t(now);
    std::cout << std::put_time(std::localtime(&now_c), "[%y/%m/%d %H:%M:%S.") 
              << std::setw(3) << std::setfill('0') << duration_cast<milliseconds>(millis).count() 
              << "]";
}

Logger::Logger() {
    
}

Logger::~Logger() {
    if(lm) {
        finish();
    }
}

void Logger::finish() {
    delete lm;
    lm = nullptr;
}

std::mutex& Logger::get_lock() {
    return Logger::mutex;
}

std::mutex Logger::mutex{};
thread_local Logger internal::log;

}
