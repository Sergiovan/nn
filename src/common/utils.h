#pragma once

#include "common/convenience.h"

#include <type_traits>
#include <iostream>
#include <stdint.h>
#include <sstream>
#include <mutex>

namespace logger {

namespace color {
    constexpr char reset  [] = "\033[0m";
    constexpr char red    [] = "\033[31m";
    constexpr char green  [] = "\033[32m";
    constexpr char yellow [] = "\033[33m";
    constexpr char blue   [] = "\033[34m";
    constexpr char magenta[] = "\033[35m";
    constexpr char cyan   [] = "\033[36m";
    constexpr char white  [] = "\033[37m";
}
    
class Logger;
class LoggerMessage;

struct ender {};
struct nender {};

template <typename T>
struct get_res { using res = LoggerMessage&; };

template <>
struct get_res<ender> { using res = void; };

template <>
struct get_res<nender> { using res = void; };

struct starter          {static constexpr char pre[] = " \033[1;37m";};
struct sinfo  : starter {static constexpr char pre[] = " \033[1;32m[INFO ] ";};
struct sdebug : starter {static constexpr char pre[] = " \033[1;36m[DEBUG] ";};
struct swarn  : starter {static constexpr char pre[] = " \033[1;33m[WARN ] ";};
struct serror : starter {static constexpr char pre[] = " \033[1;31m[ERROR] ";};

class LoggerMessage {
public:
    LoggerMessage(Logger& logger, bool timestamp = true, const std::string& pre = " ");
    ~LoggerMessage();
    
    template<typename T>
    typename get_res<T>::res operator<<(const T& other);
    
private:
    void put_timestamp();
    
    Logger& logger;
    bool timestamp;
    std::string pre;
    std::stringstream ss{};
};


class Logger {
public:
    Logger();
    ~Logger();
    
    void finish();
    
    template<typename T>
    typename get_res<T>::res operator<<(const T& other) {
        using BT = std::remove_cv_t<T>;
        if constexpr(std::is_same_v<BT, ender>) {
            if(lm) {
                finish();
            }
        } else if constexpr(std::is_same_v<BT, nender>) {
            if(lm) {
                *lm << '\n'; 
                finish();
            }
        } else if constexpr(std::is_base_of_v<starter, BT> || std::is_same_v<BT, starter>) {
            if(!lm) {
                lm = new LoggerMessage(*this, true, BT::pre);
            }
            return *lm;
        } else {
            if(!lm) {
                lm = new LoggerMessage(*this);
            }
            return (*lm << other);
        }
    }
    
    std::mutex& get_lock();
private:
    LoggerMessage* lm{nullptr};
    static std::mutex mutex;
};

template<typename T>
typename get_res<T>::res LoggerMessage::operator<<(const T& other) {
    if constexpr(std::is_same_v<std::remove_cv_t<T>, ender>) {
        logger.finish();
    } else if constexpr(std::is_same_v<std::remove_cv_t<T>, nender>) {
        ss << '\n';
        logger.finish();
    } else {
        ss << other;
        return *this;
    }
}

namespace internal {
    extern thread_local Logger log;
    constexpr starter start;
    constexpr sdebug debug;
    constexpr sinfo info;
    constexpr swarn warn;
    constexpr serror error;
}

constexpr ender end;
constexpr nender nend;
inline LoggerMessage& log() { return internal::log << internal::start; }
inline LoggerMessage& debug() { return internal::log << internal::debug; }
inline LoggerMessage& info() { return internal::log << internal::info; }
inline LoggerMessage& warn() { return internal::log << internal::warn; }
inline LoggerMessage& error() { return internal::log << internal::error; }

}

namespace utils {
    inline u8 utflen(u8 c) {
        if (c > 0b10000000) {
            return 1;
        } else if (c > 0b11100000) {
            return 2;
        } else if (c > 0b11110000) {
            return 3;
        } else if (c > 0b11111000) {
            return 4;
        } else {
            return 0;
        }
    }
}
