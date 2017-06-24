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
private:
    ReaderType type;
    std::variant<std::string, std::ifstream> stream;
};