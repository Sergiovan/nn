#pragma once

#include "common/token.h"
#include "common/list.h"

class token_stream : public list<token> {
public:
    token_stream(const std::string& filename);
    token_stream(const std::string& name, const std::string& content);
    ~token_stream();
    
    void read();
    std::string get_name();
    void split(token* tok, u64 at);
private:
    void read_one();
    
    void add(char c);
    
    token* create(token_type tt, u64 start, u64 value = 0);
    
    std::string name{};
    std::string content{};
    u64 contentptr{0};
    u64 columnstart{0};
    u64 line{0};
    
    char* buffer{nullptr};
    u64 buffsize{0};
    u64 buffptr{0};

    bool done{false};
    
};
