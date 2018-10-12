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
    
    std::string get_file_line(int context_back = -1, int context_forward = -1, int max_chars = 80);
    std::string get_file_name();
    std::string get_file_position();
private:
    ReaderType type;
    std::variant<std::string, std::ifstream> stream;
    unsigned long index = 0, line = 1, column = 1;
    bool finished = false;
    
    std::string filename;
};