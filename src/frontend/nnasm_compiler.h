#pragma once
#include "backend/nnasm.h"

#include <variant>
#include <iostream>
#include <fstream>

enum class nnasm_token_type {
    INVALID, OPCODE, REGISTER, MEMORY, IMMEDIATE,
    INDIRECT, VALUE, STRING, IDEN, TYPE, END, ERROR
};

enum class nnasm_type {
    U8, U16, U32, U64,
    S8, S16, S32, S64,
    F32, F64, NONE
};

std::ostream& operator<<(std::ostream& os, nnasm_token_type t);
std::ostream& operator<<(std::ostream& os, nnasm_type t);

struct nnasm_token;

struct nnasm_token_opcode {
    nnasm::opcode opcode;
};

struct nnasm_token_register {
    u8 floating : 1;
    u8 number : 7;
};

struct nnasm_token_indirect {
    u64* tok;
};

struct nnasm_token_memory {
    nnasm_token_type data;
    nnasm_token_type offset;
    bool offset_signed;
    union {
        u64 imm_data;
        nnasm_token_register reg_data;
        nnasm_token_indirect ind_data;
    };
    union {
        u64 imm_offset;
        nnasm_token_register reg_offset;
        nnasm_token_indirect ind_offset;
    };
};

struct nnasm_token_immediate {
    u64 data;
};

struct nnasm_token_string {
    std::string str;
};

struct nnasm_token_iden {
    std::string iden;
};

struct nnasm_token_nnasm_type {
    nnasm_type type;
};

struct nnasm_token_end {};

using nnasm_token_variant = std::variant<nnasm_token_opcode, nnasm_token_register, nnasm_token_memory, 
                                         nnasm_token_immediate,  nnasm_token_indirect, nnasm_token_string, 
                                         nnasm_token_iden, nnasm_token_nnasm_type, nnasm_token_end>;

struct nnasm_token {
    nnasm_token_type type{nnasm_token_type::INVALID};
    nnasm_token_variant data;
    
    bool is_opcode();
    bool is_register();
    bool is_memory();
    bool is_immediate();
    bool is_indirect();
    bool is_string();
    bool is_iden();
    bool is_type();
    bool is_end();
    bool is_error();
    
    nnasm_token_opcode&     as_opcode();
    nnasm_token_register&   as_register();
    nnasm_token_memory&     as_memory();
    nnasm_token_immediate&  as_immediate();
    nnasm_token_indirect&   as_indirect();
    nnasm_token_string&     as_string();
    nnasm_token_iden&       as_iden();
    nnasm_token_nnasm_type& as_type();
    nnasm_token_end&        as_end();
    
    std::string print();
};

struct nnexe_header {
    char magic[4] = {'N', 'N', 'E', 'P'};
    u32 version = 0;
    u64 code_start = 128;
    u64 data_start = 0;
    u64 size = 0;
    u64 initial = 4 << 20;
};

class nnasm_compiler {
public:
    nnasm_compiler(const std::string& file);
    ~nnasm_compiler();
    
    void compile();
    
    void first_pass();
    void second_pass();
    
    void print_errors();
    
    u8* get_program();
    u8* move_program();
    u64 get_size();
    
    void store_to_file(const std::string& filename);
    
private:
    nnasm_token next();
    nnasm_token to_token(const std::string& tok);
    void error(const std::string& err);
    
    std::ifstream stream;
    
    bool done{false};
    
    std::string source{};
    u64 file_pos{0};
    u64 file_line{1};
    u64 file_col{1};
    
    nnasm_type prev_type{nnasm_type::NONE};
    
    std::vector<std::string> errors{};
    
    std::vector<nnasm_token*> tokens{};
    dict<u64*> indirects{};
    dict<nnasm_token> values{};
    
    u8* program{nullptr};
    u64 program_size{0};
};
