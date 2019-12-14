#pragma once
#include "backend/nnasm.h"

#include <variant>
#include <iostream>
#include <fstream>
#include <set>
#include <cstring>
#include <type_traits>

namespace nnasm {

enum class token_type {
    INVALID, OPCODE, REGISTER, MEMORY, IMMEDIATE,
    STRING, IDEN, TYPE, END, ERROR
};
    
struct token;

struct token_opcode {
    nnasm::opcode opcode;
};

struct token_register {
    data_type type;
    u8 floating : 1;
    u8 number : 7;
};

struct token_immediate {
    data_type type;
    u64 data;
};

struct token_iden {
    std::string iden;
};

using memory_variant = std::variant<token_immediate, token_register, token_iden>;

struct token_memory {
    data_type type;
    token_type location;
    token_type offset;
    bool offset_negated;
    memory_variant location_data;
    memory_variant offset_data;
};

struct token_string {
    std::string str;
};

struct token_nnasm_type {
    data_type type;
};

struct token_end {};

using token_variant = std::variant<token_opcode, token_register, token_memory, 
                                    token_immediate, token_string, 
                                    token_iden, token_nnasm_type, token_end>;

struct token {
    token_type type{token_type::INVALID};
    token_variant data;
    
    bool is_opcode();
    bool is_register();
    bool is_memory();
    bool is_immediate();
    bool is_string();
    bool is_iden();
    bool is_type();
    bool is_end();
    bool is_error();
    
    token_opcode&     as_opcode();
    token_register&   as_register();
    token_memory&     as_memory();
    token_immediate&  as_immediate();
    token_string&     as_string();
    token_iden&       as_iden();
    token_nnasm_type& as_type();
    token_end&        as_end();
    
    data_type get_type();
    std::string print();
};

class compiler {
public:
    compiler(const std::string& file);
    ~compiler();
    
    void compile();
    
    void first_pass(); // Build program
    void second_pass(); // Fill in first pass holes
    
    void print_errors();
    
    u8* get_program();
    u8* move_program();
    u64 get_size();
    
    void store_to_file(const std::string& filename);
    
private:
    token next(bool nested = false);
    token to_token(const std::string& tok);
    void error(const std::string& err);
    
    std::ifstream stream;
    
    bool done{false};
    
    std::string source{};
    u64 file_pos{0};
    u64 file_line{1};
    u64 file_col{1};
    
    std::vector<std::string> errors{};
    
    dict<token> values{};
    
    struct dbe {
        u64 value;
        u64 length = 0;
        bool defined = false;
    };
    
    struct finish {
        token tok;
        u64 location;
        bool data{false};
    };
    
    dict<dbe> idens{};
    std::vector<finish> unfinished{};
    
    u64 pc{0}; // Program counter
    u64 dc{0}; // Data counter
    
    u8* program{nullptr};
    u64 program_size{0};
    
    u8* data{nullptr};
    u64 data_size{0};
    
    inline u64 to_align(u64 from, u8 to) {
        const u8 modulo = from % to;
        return (to - modulo) * (modulo != 0);
    }
    
    inline void insert_p(const u8* ptr, u64 size) {
        const u8 divisor = (size > 4) ? 8 : (size > 2) ? 4 : (size > 1) ? 2 : 1;
        const u8 modulo = pc % divisor;
        pc += (divisor - modulo) * (modulo != 0);
        if (program_size < pc + size) {
            u64 prev_size = program_size;
            program_size += size;
            program_size *= 2;
            u8* buff = new u8[program_size];
            if constexpr(__debug) {
                std::memset(buff + prev_size, 0, program_size);
            }
            std::memcpy(buff, program, prev_size);
            delete [] program;
            program = buff;
        }
        std::memcpy(program + pc, ptr, size);
        pc += size;
    }
    
    inline void insert_p(u64 amount) {
        if (program_size < pc + amount) {
            u64 prev_size = program_size;
            program_size += amount;
            program_size *= 2;
            u8* buff = new u8[program_size];
            if constexpr(__debug) {
                std::memset(buff + prev_size, 0, program_size);
            }
            std::memcpy(buff, program, prev_size);
            delete [] program;
            program = buff;
        }
        std::memset(program + pc, 0, amount);
        pc += amount;
    }
    
    inline void insert_d_no_align(const u8* ptr, u64 size) {
        if (data_size < dc + size) {
            u64 prev_size = data_size;
            data_size += size;
            data_size *= 2;
            u8* buff = new u8[data_size];
            if constexpr(__debug) {
                std::memset(buff + prev_size, 0, data_size);
            }
            std::memcpy(buff, data, prev_size);
            delete [] data;
            data = buff;
        }
        std::memcpy(data + dc, ptr, size);
        dc += size;
    }
    
    inline void insert_d(const u8* ptr, u64 size) {
        const u8 divisor = (size > 4) ? 8 : (size > 2) ? 4 : (size > 1) ? 2 : 1;
        const u8 modulo = dc % divisor;
        dc += (divisor - modulo) * (modulo != 0);
        if (data_size < dc + size) {
            u64 prev_size = data_size;
            data_size += size;
            data_size *= 2;
            u8* buff = new u8[data_size];
            if constexpr(__debug) {
                std::memset(buff + prev_size, 0, data_size);
            }
            std::memcpy(buff, data, prev_size);
            delete [] data;
            data = buff;
        }
        std::memcpy(data + dc, ptr, size);
        dc += size;
    }
};

}

std::ostream& operator<<(std::ostream& os, nnasm::token_type t);
std::ostream& operator<<(std::ostream& os, nnasm::data_type t);
