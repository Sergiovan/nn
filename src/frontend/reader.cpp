#include "frontend/reader.h"

#include <filesystem>
#include "common/utils.h"

namespace fs = std::filesystem;

reader::reader(const std::string& str, bool is_file) {
    if(is_file) {
        type = ReaderType::FILE;
        try {
            if(!fs::exists(str)) {
                logger::error() << "File \"" << str << "\" does not exist" << logger::nend;
                done = true;
                type = ReaderType::INVALID;
                return;
            }
        } catch (...) {
            logger::error() << "Error checking existence of \"" << str << "\"" << logger::nend;
            done = true;
            type = ReaderType::INVALID;
            return;
        }
        source = fs::path(str).filename().string();
        stream = std::ifstream(str, std::ifstream::in | std::ifstream::binary);
        std::ifstream& s = std::get<std::ifstream>(stream);
        if(!s || !s.is_open()) {
            logger::error() << "File \"" << str << "\" could not be opened" << logger::nend;
            done = true;
            type = ReaderType::INVALID;
            return;
        }
    } else {
        type = ReaderType::STRING;
        stream = str;
        source = "<input>";
    }
}

reader* reader::from_file(const std::string& str) {
    return new reader{str, true};
}

reader* reader::from_string(const std::string& str) {
    return new reader{str, false};
}

char reader::next() {
    if (done) {
        return EOF;
    }
    char c;
    if(type == ReaderType::FILE) {
        std::ifstream& s = std::get<std::ifstream>(stream);
        s.get(c);
        if(c == EOF || s.eof()) {
            done = true;
            c = EOF;
        }
    } else if(type == ReaderType::STRING) {
        std::string& s = std::get<std::string>(stream);
        if(index >= s.length()) {
            done = true;
            c = EOF;
        } else {
            c = s[index];
        }
    } else {
        logger::error() << "Invalid reader" << logger::nend;
        return EOF;
    }
    
    prev_column = column;
    ++index; ++column;
    if(c == '\n') {
        ++line;
        column = 1;
    }
    
    return c;
}

char reader::peek(int i) {
    if(done) {
        return EOF;
    }
    
    if(type == ReaderType::FILE) {
        if(i) {
            std::ifstream& s = std::get<std::ifstream>(stream); // TODO Better buffer
            u64 pos = s.tellg();
            s.seekg(i, std::ios::cur);
            char c = s.peek();
            s.seekg(pos, std::ios::beg);
            return c;
        } else {
            return std::get<std::ifstream>(stream).peek();
        }
    } else if(type == ReaderType::STRING) {
        std::string& s = std::get<std::string>(stream);
        int pos = i + index;
        if (pos < 0 || pos >= s.length()) {
            return EOF;
        } else {
            return s[pos];
        }
    } else {
        logger::error() << "Invalid reader" << logger::nend;
        return EOF;
    }
}

u64 reader::get_index() {
    return index;
}

u64 reader::get_line() {
    return line;
}

u64 reader::get_column() {
    return column;
}

bool reader::is_done() {
    return done;
}

u64 reader::get_prev_index() {
    return index - 1;
}

u64 reader::get_prev_line() {
    if (prev_column >= column) {
        return line - 1;
    } else {
        return line;
    }
}

u64 reader::get_prev_column() {
    return prev_column;
}

std::string reader::get_file_line(int context_back, int context_forward, int max_chars) {
    // TODO
    return "";
}

std::string reader::get_source() {
    return source;
}

std::string reader::get_info() {
    std::stringstream ss;
    ss << source << ":" << line << ":" << column;
    return ss.str();
}
