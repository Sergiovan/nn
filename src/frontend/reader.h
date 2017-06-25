#pragma once

#include <fstream>
#include <string>
#include <variant>

enum class ReaderType {
    STRING, FILE
};

class reader {
public:
    reader(std::string str, bool is_file = true);
    
    char next();
    char peek();
    
    unsigned long get_index();
    unsigned long get_line();
    unsigned long get_column();
    
    bool has_finished();
private:
    ReaderType type;
    std::variant<std::string, std::ifstream> stream;
    unsigned long index = 0, line = 1, column = 1;
    bool finished = false;
};