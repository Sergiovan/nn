#pragma once

#include <fstream>
#include <string>
#include <variant>

#include "common/convenience.h"

enum class ReaderType { STRING, FILE, INVALID };

class reader {
public:
    static reader* from_string(const std::string& str);
    static reader* from_file(const std::string& file);
    
    char next();
    char peek(int i = 0);
    
    u64 get_index();
    u64 get_line();
    u64 get_column();
    bool is_done();
    
    u64 get_prev_index();
    u64 get_prev_line();
    u64 get_prev_column();
    
    std::string get_file_line(int context_back = -1, int context_forward = -1, int max_chars = 80);
    std::string get_source();
    std::string get_info();
    
private:
    reader(const std::string& str, bool is_file);
    
    ReaderType type{ReaderType::INVALID};
    std::variant<std::string, std::ifstream> stream;
    std::string source;
    u64 index{0}, line{1}, column{1};
    u64 prev_column{1};
    bool done{false};
};
