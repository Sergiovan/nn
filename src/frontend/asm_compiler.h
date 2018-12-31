#pragma once

#include <fstream>
#include <vector>
#include "common/asm.h"
#include "common/convenience.h"

/* Please note: This entire class is fucking garbage, a quick hack. 
 * Must be revisited and rewritten entirely. */

struct nnexe_header {
    char magic[4] = {'N', 'N', 'E', 'P'};
    u32 version = 0;
    u64 code_start = 128;
    u64 data_start = 0;
    u64 size = 0;
    u64 initial = 4 << 20;
};

class asm_compiler {
public:
    asm_compiler(const std::string& filename);
    ~asm_compiler();
    
    // Rule of 5 not needed for now, nothing fancy
    
    void compile();
    void print_errors();
    
    u8* get(); // Still owned by asm_compiler, do not delete
    u8* move(); // Delete when done plox
    u64 size();
    void store_to_file(const std::string& filename);
    
private:
    struct db_value {
        u8* data;
        u64 size;
        u64 addr;
    };
    
    template <typename T>
    void add(T data) {
        add(reinterpret_cast<u8*>(&data), sizeof(T));
    }
    
    void add(u8* data, u64 length);
    
    nnasm::instruction next_instruction();
    nnasm::op::code    next_opcode();
    u64                next_value(nnasm::instruction& ins, u8 index, std::string* strptr = nullptr, bool allow_pending = true, bool allow_database = true);
    u64                value_from_string(const std::string& val, nnasm::instruction_code::operand& op);
    
    char               next_char();
    bool               peek_separator();
    bool               peek_eol();
    
    
    void error(const std::string& msg);
    
    std::string filename;
    std::ifstream file;
    bool ready{false};
    u64 line{1};
    
    std::vector<std::string> errors{};
    
    std::map<std::string, u64> labels{};
    std::map<std::string, db_value> database{};
    std::vector<std::pair<u64, std::string>> pending{};
    
    std::map<std::string, std::string> literals{};
    
    u64 addr{0};
    u64 shaddr{0}; // Shadow address, where things should be, not where they are
    
    u8* compiled{nullptr};
    u64 compiled_size{0};
    
    u8* buffer{new u8[sizeof(nnasm::instruction) * 2]};
};
