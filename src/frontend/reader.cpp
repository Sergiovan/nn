#include "common/filesystem_include.h"
#include "common/logger.h"
#include "reader.h"

namespace fs = std::filesystem;

reader::reader(std::string str, bool is_file) {
    if(is_file) {
        bool exists;
        try {
            exists = fs::exists(str);
        } catch(...) {
            exists = false;
        }
        filename = std::filesystem::path(str.c_str()).filename().string();
        if(exists) {
            stream = std::ifstream(str, std::ifstream::in | std::ifstream::binary);
            std::ifstream& s = std::get<1>(stream);
            if(!s || !s.is_open()) {
                Logger::error("File", str, "could not be opened correctly");
                finished = true;
            } else {
                type = ReaderType::FILE;
            }
        } else {
            Logger::error("File", str, "does not exist!");
            finished = true;
        }
    } else {
        stream = str;
        filename = "<user input>";
        type = ReaderType::STRING;
    }
}

char reader::next() {
    if(finished) {
        return EOF;
    }
    char c;
    if(type == ReaderType::FILE) {
        std::ifstream& s = std::get<1>(stream);
        s.get(c);
        if(c == EOF || s.eof()) {
            finished = true;
            c = EOF;
        }
    } else if(type == ReaderType::STRING) {
        std::string& s = std::get<0>(stream);
        if(index >= s.length()) {
            finished = true;
            c = EOF;
        } else {
            c = s.at(index);
        }
    } else {
        Logger::error("The world as we know it has come to an end");
        std::abort();
    }
    
    index++;
    column++;
    if(c == '\n') {
        line++;
        column = 1;
    }
    
    return c;
}

char reader::peek() {
    if(finished) {
        return EOF;
    }
    
    if(type == ReaderType::FILE) {
        return std::get<1>(stream).peek();
    } else if(type == ReaderType::STRING) {
        std::string& s = std::get<0>(stream);
        if(index >= s.length()) {
            return EOF;
        } else {
            return s.at(index);
        }
    } else {
        Logger::error("A random bit toggles in the distance, and a hurricane destroys Myanmar...");
        std::abort();
    }
}

unsigned long reader::get_index() {
    return index;
}

unsigned long reader::get_line() {
    return line;
}

unsigned long reader::get_column() {
    return column;
}

bool reader::has_finished() {
    return finished;
}

std::string reader::get_file_line(int context_back, int context_forward, int max_chars) {
    std::string str;
    if(type == ReaderType::STRING) {
        str = std::get<0>(stream);
    } else {
        auto& fs = std::get<1>(stream);
        fs.seekg(index - column + 1);
        std::getline(fs, str);
        fs.seekg(index);
    }
    if(str.length() < max_chars) {
           return str;
       } else {
           int ll = column;
           int rl = column;
           ll -= context_back > -1 ? context_back : 0;
           rl += context_forward > -1 ? context_forward: 0;
           int total = rl - ll;
           if(total >= max_chars) {
               total = max_chars;
           }
           int addage = (max_chars - total) / 2;
           ll -= addage;
           rl += addage;
           if(ll < 0) {
               rl += -ll;
               ll = 0;
           } 
           if(rl >= str.length()) {
               ll -= rl - (str.length() + 1);
               rl = str.length() - 1;
           }
           return str.substr(ll, rl - ll);
       }
}

std::string reader::get_file_name() {
    return filename;
}

std::string reader::get_file_position() {
    std::stringstream ss;
    ss << filename << ":" << line << ":" << column;
    return ss.str();
}