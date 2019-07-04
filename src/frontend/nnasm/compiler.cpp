#include "frontend/nnasm/compiler.h"

#include "common/convenience.h"
#include "common/utils.h"

#include <iomanip>
#include <filesystem>
#include <regex>
#include <sstream>

using namespace std::string_literals;
using namespace nnasm;
namespace fs = std::filesystem;

std::ostream& operator<<(std::ostream& os, token_type t) {
    switch (t) {
        case token_type::INVALID:
            return os << "Invalid";
        case token_type::OPCODE:
            return os << "Opcode";
        case token_type::REGISTER:
            return os << "Register";
        case token_type::MEMORY:
            return os << "Memory";
        case token_type::IMMEDIATE:
            return os << "Immediate";
        case token_type::STRING:
            return os << "String";
        case token_type::IDEN:
            return os << "Iden";
        case token_type::TYPE:
            return os << "Type";
        case token_type::END:
            return os << "End";
        case token_type::ERROR:
            return os << "Error";
        default:
            return os << "???";
    }
}

std::ostream& operator<<(std::ostream& os, data_type t) {
    switch (t) {
        case data_type::U8:
            return os << "u8";
        case data_type::U16:
            return os << "u16";
        case data_type::U32:
            return os << "u32";
        case data_type::U64:
            return os << "u64";
        case data_type::S8:
            return os << "s8";
        case data_type::S16:
            return os << "s16";
        case data_type::S32:
            return os << "s32";
        case data_type::S64:
            return os << "s64";
        case data_type::F32:
            return os << "f32";
        case data_type::F64:
            return os << "f64";
        case data_type::NONE:
            return os << "u64";
        default:
            return os << "???";
    }
}

bool token::is_opcode() {
    return type == token_type::OPCODE;
}

bool token::is_register() {
    return type == token_type::REGISTER;
}

bool token::is_memory() {
    return type == token_type::MEMORY;
}

bool token::is_immediate() {
    return type == token_type::IMMEDIATE;
}

bool token::is_string() {
    return type == token_type::STRING;
}

bool token::is_iden() {
    return type == token_type::IDEN;
}

bool token::is_type() {
    return type == token_type::TYPE;
}

bool token::is_end() {
    return type == token_type::END;
}

bool token::is_error() {
    return type == token_type::ERROR;
}

token_opcode& token::as_opcode() {
    return std::get<token_opcode>(data);
}

token_register& token::as_register() {
    return std::get<token_register>(data);
}

token_memory& token::as_memory() {
    return std::get<token_memory>(data);
}

token_immediate& token::as_immediate() {
    return std::get<token_immediate>(data);
}

token_string& token::as_string() {
    return std::get<token_string>(data);
}

token_iden& token::as_iden() {
    return std::get<token_iden>(data);
}

token_nnasm_type& token::as_type() {
    return std::get<token_nnasm_type>(data);
}

token_end& token::as_end() {
    return std::get<token_end>(data);
}

data_type token::get_type() {
    switch (type) {
        case token_type::REGISTER:  return as_register().type;
        case token_type::IMMEDIATE: return as_immediate().type;
        case token_type::MEMORY:    return as_memory().type;
        default: return data_type::NONE;
    }
}

std::string token::print() {
    std::stringstream ss{};
    ss << type << ": ";
    switch (type) {
        case token_type::INVALID:
            ss << "?????";
            break;
        case token_type::OPCODE:
            ss << nnasm::op_to_name.at(as_opcode().opcode);
            break;
        case token_type::REGISTER: {
            auto& reg = as_register();
            ss << reg.type << " "; 
            if (reg.floating) {
                ss << "$f" << (u16) reg.number;
            } else {
                if (reg.number == 17) {
                    ss << "$pc";
                } else if (reg.number == 18) {
                    ss << "$sf";
                } else if (reg.number == 19) {
                    ss << "$sp";
                } else {
                    ss << "$r" << (u16) reg.number;
                }
            }
            break;
        }
        case token_type::MEMORY: {
            auto& mem = as_memory();
            ss << mem.type << " [";
            if (mem.location == token_type::REGISTER) {
                ss << token{mem.location, std::get<token_register>(mem.location_data)}.print();
            } else if (mem.location == token_type::IMMEDIATE) {
                ss << token{mem.location, std::get<token_immediate>(mem.location_data)}.print();
            } else {
                ss << token{mem.location, std::get<token_iden>(mem.location_data)}.print();
            }
            
            if (mem.offset != token_type::END) {
                if (mem.offset_negated) {
                    ss << " - ";
                } else {
                    ss << " + ";
                }
                
                if (mem.offset == token_type::REGISTER) {
                    ss << token{mem.offset, std::get<token_register>(mem.offset_data)}.print();
                } else if (mem.offset == token_type::IMMEDIATE) {
                    ss << token{mem.offset, std::get<token_immediate>(mem.offset_data)}.print();
                } else {
                    ss << token{mem.offset, std::get<token_iden>(mem.offset_data)}.print();
                }
            }
            ss << "]";
            break;
        }
        case token_type::IMMEDIATE:
            ss << as_immediate().type << " " << as_immediate().data;
            break;
        case token_type::STRING:
            ss << as_string().str;
            break;
        case token_type::IDEN:
            ss << as_iden().iden;
            break;
        case token_type::TYPE:
            ss << as_type().type;
            break;
        case token_type::END:
            ss << "EOF";
            break;
        case token_type::ERROR:
            break;
        default:
            ss << "???";
            break;
    }
    
    return ss.str();
}

compiler::compiler(const std::string& file) {
    try {
        if(!fs::exists(file)) {
            logger::error() << "File \"" << file << "\" does not exist" << logger::nend;
            done = true;
            return;
        }
    } catch (...) {
        logger::error() << "Error checking existence of \"" << file << "\"" << logger::nend;
        done = true;
        return;
    }
    source = fs::path(file).filename().string();
    stream = std::ifstream(file, std::ifstream::in | std::ifstream::binary);
    if(!stream || !stream.is_open()) {
        logger::error() << "File \"" << file << "\" could not be opened" << logger::nend;
        done = true;
        return;
    }
}

compiler::~compiler() {
    if (program) {
        delete [] program;
    }
    
    if (data) {
        delete [] data;
    }
}

void compiler::compile() {
    if (done) {
        return;
    }
    
    program = new u8[1024];
    program_size = 1024;
    if constexpr(__debug) {
        std::memset(program, 0, program_size);
    }
    pc = sizeof(nnexe_header);
    
    data = new u8[1024];
    data_size = 1024;
    if constexpr(__debug) {
        std::memset(data, 0, data_size);
    }
    dc = 0;
    
    first_pass();
    second_pass();
    
}

void compiler::first_pass() {
    if (done) {
        return;
    }
    
    auto expect = [this] (token& tok, token_type type) {
        if (tok.type != type) {
            std::stringstream ss{};
            ss << "Expected " << type << " but got \"" << tok.print() << "\" instead";
            error(ss.str());
            return false;
        }
        return true;
    };
    
    auto fmts = format::get_formats();
    
    token tok = next();
    while (!tok.is_end()) {
        
        if (!expect(tok, token_type::OPCODE)) {
            tok = next();
            continue;
        }
        
        switch (tok.as_opcode().opcode) {
            case opcode::LBL: {
                auto idn = next();
                if (!expect(idn, token_type::IDEN)) {
                    tok = next();
                    continue;
                }
                auto& iden = idn.as_iden().iden;
                if (auto ptr = idens.find(iden); ptr == idens.end()) {
                    idens.insert({iden, {pc, 0, true}});
                } else {
                    std::stringstream ss{};
                    ss << "Identifier " << iden << " already exists";
                    error(ss.str());
                }
                tok = next();
                continue;
            }
            case opcode::VAL: {
                auto idn = next(); 
                if (!expect(idn, token_type::IDEN)) {
                    tok = next();
                    continue;
                }
                auto val = next();
                if (val.is_end() || val.is_error() || val.is_opcode()) {
                    std::stringstream ss{};
                    ss << "Value cannot be " << val.type;
                    error(ss.str());
                    continue;
                }
                values.insert({idn.as_iden().iden, val});
                tok = next();
                continue;
            }
            case opcode::DB: {
                auto idn = next();
                if (!expect(idn, token_type::IDEN)) {
                    tok = next();
                    continue;
                }
                auto& iden = idn.as_iden().iden;
                auto ptr = idens.find(iden);
                if (ptr == idens.end()) {
                    ptr = idens.insert({idn.as_iden().iden, {dc}}).first;
                } else {
                    std::stringstream ss{};
                    ss << "Identifier " << iden << " already exists";
                    error(ss.str());
                }
                tok = next();
                
                // Align to 8
                u64 val = 0;
                insert_d((u8*) &val, 8);
                dc -= 8;
                u64 dcs = dc;
                
                while (!tok.is_end() && !tok.is_opcode()) {
                    if (tok.is_error()) {
                        tok = next();
                        continue;
                    }
                    if (tok.is_string()) {
                        insert_d_no_align((const u8*) tok.as_string().str.c_str(), tok.as_string().str.length());
                    } else {
                        if (!expect(tok, token_type::IMMEDIATE)) {
                            tok = next();
                            continue;
                        }
                        u64 val = tok.as_immediate().data;
                        insert_d_no_align((u8*) &val, data_type_size(tok.as_immediate().type));
                    }
                    
                    tok = next();
                }
                u64 dce = dc;
                if (dc < dcs + 8) {
                    dc = dcs + 8;
                }
                
                ptr->second.length = dce - dcs;
                
                continue;
            }
            case opcode::DBS: { // TODO
                auto idn = next();
                if (!expect(idn, token_type::IDEN)) {
                    tok = next();
                    continue;
                }
                auto& iden = idn.as_iden().iden;
                auto ptr = idens.find(iden);
                if (ptr == idens.end()) {
                    ptr = idens.insert({idn.as_iden().iden, {dc}}).first;
                } else {
                    std::stringstream ss{};
                    ss << "Identifier " << iden << " already exists";
                    error(ss.str());
                }
                tok = next();
                
                // Align to 8
                u64 val = 0;
                insert_d((u8*) &val, 8);
                dc -= 8;
                u64 dcs = dc;
                
                while (!tok.is_end() && !tok.is_opcode()) {
                    if (tok.is_error()) {
                        tok = next();
                        continue;
                    }
                    if (tok.is_string()) {
                        insert_d_no_align((const u8*) tok.as_string().str.c_str(), tok.as_string().str.length());
                    } else {
                        if (!expect(tok, token_type::IMMEDIATE)) {
                            tok = next();
                            continue;
                        }
                        u64 val = tok.as_immediate().data;
                        insert_d_no_align((u8*) &val, data_type_size(tok.as_immediate().type));
                    }
                    
                    tok = next();
                }
                u64 dce = dc;
                if (dc < dcs + 8) {
                    dc = dcs + 8;
                }
                
                ptr->second.length = dce - dcs;
                
                continue;
            }
            default:
                break;
        }
        
        format::instruction instr;
        
        opcode code = tok.as_opcode().opcode;
        u8 op = 0;
        token ops[3] = {};
        tok = next();
        while (!tok.is_end() && !tok.is_opcode()) {
            if (tok.is_error()) {
                tok = next();
                continue;
            }
            
            if (op > 2) {
                std::stringstream ss{};
                ss << "Too many operands: " << tok.print();
                error(ss.str());
                
                tok = next();
                continue;
            }
            
            if (tok.is_immediate()) {
                instr.op[op].imm = true;
            } else if (tok.is_register()) {
                instr.op[op].reg = true;
            } else if (tok.is_memory()) {
                instr.op[op].mem = true;
            } else if (tok.is_iden()) {
                instr.op[op].imm = true;
                instr.op[op]._u64 = true;
            } else {
                std::stringstream ss{};
                ss << "Invalid token " << tok.print();
                error(ss.str());
            }
            
            if (!tok.is_iden()) {
                switch (tok.get_type()) {
                    case data_type::U8:  instr.op[op]._u8  = true; break;
                    case data_type::U16: instr.op[op]._u16 = true; break;
                    case data_type::U32: instr.op[op]._u32 = true; break;
                    case data_type::U64: instr.op[op]._u64 = true; break;
                    case data_type::S8:  instr.op[op]._s8  = true; break;
                    case data_type::S16: instr.op[op]._s16 = true; break;
                    case data_type::S32: instr.op[op]._s32 = true; break;
                    case data_type::S64: instr.op[op]._s64 = true; break;
                    case data_type::F32: instr.op[op]._f32 = true; break;
                    case data_type::F64: instr.op[op]._f64 = true; break;
                    case data_type::NONE: {
                        std::stringstream ss{};
                        ss << "Type for " << tok.print() << " not defined";
                        error(ss.str());
                        break;
                    }
                }
            }
            
            ops[op] = tok;
            ++op;
            tok = next();
        }
        
        auto fmt_candidates = fmts[(u64) code];
        bool found = false;
        opcode_internal icode{};
        for (auto& [fmt, code] : fmt_candidates) {
            if ((fmt.raw & instr.raw) == instr.raw) {
                found = true;
                icode = code;
                break;
            }
        }
        
        if (!found) {
            std::stringstream ss{};
            ss << "Invalid format for " << code << ": 0x" << std::hex << instr.raw;
            error(ss.str());
            tok = next();
            continue;
        }
        
        // Insert opcode 
        insert_p((u8*) &icode, sizeof(u16));
        for (u8 i = 0; i < 3; ++i) {
            auto& op = ops[i];
            if (op.type == token_type::INVALID) {
                break;
            } else if (op.is_immediate()) {
                insert_p((u8*) &op.as_immediate().data, data_type_size(op.as_immediate().type));
            } else if (op.is_register()) {
                u8 rval = op.as_register().number + (op.as_register().floating * 19);
                insert_p(&rval, 1);
            } else if (op.is_memory()) {
                auto& mem = op.as_memory();
                mem_hdr hdr{};
                hdr.reg = mem.location == token_type::REGISTER;
                if (mem.offset == token_type::END) {
                    hdr.off_type = 0;
                } else if (mem.offset == token_type::REGISTER) {
                    hdr.off_type = 1 + mem.offset_negated;
                } else {
                    hdr.off_type = 3;
                }
                insert_p((u8*) &hdr, sizeof(hdr));
                u64 val = 0;
                u64 pos = 0;
                token unfinished_token;
                if (hdr.reg) {
                    u8 rval = std::get<token_register>(mem.location_data).number + (std::get<token_register>(mem.location_data).floating * 19);
                    insert_p(&rval, 1);
                } else if (mem.location == token_type::IMMEDIATE) {
                    val = std::get<token_immediate>(mem.location_data).data;
                    insert_p((u8*) &val, sizeof(val));
                    pos = pc - sizeof(val);
                } else {
                    // Always unfinished
                    insert_p((u8*) &val, sizeof(val));
                    pos = pc - sizeof(val);
                    unfinished_token = token{token_type::IDEN, std::get<token_iden>(mem.location_data)};
                }
                if (hdr.off_type == 1 || hdr.off_type == 2) { // Register or negated register
                    u8 rval = std::get<token_register>(mem.offset_data).number + (std::get<token_register>(mem.offset_data).floating * 19);
                    insert_p(&rval, 1);
                } else if (hdr.off_type == 3) { // Immediate
                    if (hdr.reg) {
                        if (mem.offset == token_type::IMMEDIATE) {
                            val = std::get<token_immediate>(mem.offset_data).data;
                            insert_p((u8*) &val, sizeof(val));
                        } else {
                            // Always unfinished
                            insert_p((u8*) &val, sizeof(val));
                            pos = pc - sizeof(val);
                            unfinished_token = token{token_type::IDEN, std::get<token_iden>(mem.location_data)};
                        }
                    } else {
                        if (unfinished_token.type != token_type::INVALID) {
                            unfinished_token = op;
                        } else if (mem.offset == token_type::IMMEDIATE) {
                            s64 neg = (s64) std::get<token_immediate>(mem.offset_data).data;
                            val += neg;
                            std::memcpy(program + pos, &val, sizeof val);
                        } else {
                            unfinished_token = op;
                        }
                    }
                }
                
                if (unfinished_token.type != token_type::INVALID) {
                    unfinished.push_back({unfinished_token, pos});
                }
                
            } else if (op.is_iden()) {
                u64 val = 0;
                insert_p((u8*) &val, sizeof(val));
                unfinished.push_back({op, pc - 8});
            }
        }
    }
}

void compiler::second_pass() {
    // Realign
    u64 val = 0;
    insert_p((u8*) &val, sizeof(val));
    pc -= 8;
    insert_d((u8*) &val, sizeof(val));
    dc -= 8;
    
    nnexe_header exe_hdr{};
    exe_hdr.data_start = pc;
    exe_hdr.size = exe_hdr.data_start + dc;
    
    std::memcpy(program, &exe_hdr, sizeof(nnexe_header));
    
    for (auto& iden : idens) {
        dbe& entry = iden.second;
        if (!entry.defined) {
            entry.value = entry.value + exe_hdr.data_start;
            entry.defined = true;
        }
    }
    for (auto& task : unfinished) {
        switch (task.tok.type) {
            case token_type::IDEN: {
                auto& str = task.tok.as_iden().iden;
                std::string_view name{str};
                if (str[0] == '~') {
                    name = std::string_view{str.c_str() + 1};
                }
                
                if (auto ptr = idens.find(name.data()); ptr != idens.end() && ptr->second.defined) {
                    if (str[0] == '~') {
                        std::memcpy((task.data ? data : program) + task.location, &ptr->second.length, sizeof ptr->second.length);
                    } else {
                        std::memcpy((task.data ? data : program) + task.location, &ptr->second.value, sizeof ptr->second.value);
                    }
                } else {
                    std::stringstream ss{};
                    ss << "Second pass: Iden \"" << name.data() << "\" could not be found";
                    error(ss.str());
                }
                break;
            }
            case token_type::MEMORY: {
                auto& mem = task.tok.as_memory();
                
                u64 val = 0;
                if (mem.location == token_type::IMMEDIATE) {
                    val = std::get<token_immediate>(mem.location_data).data;
                } else {
                    auto& str = std::get<token_iden>(mem.location_data).iden;
                    
                    std::string_view name{str};
                    if (str[0] == '~') {
                        name = std::string_view{str.c_str() + 1};
                    }
                    
                    if (auto ptr = idens.find(name.data()); ptr != idens.end() && ptr->second.defined) {
                        if (str[0] == '~') {
                            val = ptr->second.length;
                        } else {
                            val = ptr->second.value;
                        }
                    } else {
                        std::stringstream ss{};
                        ss << "Second pass: Iden \"" << name.data() << "\" could not be found";
                        error(ss.str());
                        break;
                    }
                }
                if (mem.offset == token_type::IMMEDIATE) {
                    s64 neg = (s64) std::get<token_immediate>(mem.offset_data).data;
                    val += neg;
                    std::memcpy((task.data ? data : program) + task.location, &val, sizeof val);
                } else {
                    auto& str = std::get<token_iden>(mem.offset_data).iden;
                    
                    std::string_view name{str};
                    if (str[0] == '~') {
                        name = std::string_view{str.c_str() + 1};
                    }
                    
                    if (auto ptr = idens.find(name.data()); ptr != idens.end() && ptr->second.defined) {
                        s64 neg = 0;
                        if (str[0] == '~') {
                            neg = (s64) ptr->second.length;
                        } else {
                            neg = (s64) ptr->second.value;
                        }
                        val += neg;
                        std::memcpy((task.data ? data : program) + task.location, &val, sizeof val);
                    } else {
                        std::stringstream ss{};
                        ss << "Second pass: Iden \"" << name.data() << "\" could not be found";
                        error(ss.str());
                        break;
                    }
                }
                break;
            }
            default: {
                std::stringstream ss{};
                ss << "Second pass: No data to finish token " << task.tok.print(); 
                error(ss.str());
                break;
            }
        }
    }
    
    u8* buff = new u8[dc + pc];
    std::memcpy(buff, program, pc);
    std::memcpy(buff + exe_hdr.data_start, data, dc);
    
    delete [] data;
    data = nullptr;
    data_size = 0;
    delete [] program;
    program = buff;
    program_size = dc + pc;
    
    done = true;
}

void compiler::print_errors() {
    for (auto& err : errors) {
        logger::error() << err << logger::nend;
    }
}

u8* compiler::get_program() {
    return program;
}

u8* compiler::move_program() {
    u8* buff = program;
    program = nullptr;
    return buff;
}

u64 compiler::get_size() {
    return program_size;
}

void compiler::store_to_file(const std::string& filename) {
    (void) filename;
}

namespace {
    static const std::regex memory_location_rgx{"^\\[ *([^ ]+) *(?:(\\+|-) *([^ ]+) *)?\\]$", std::regex::optimize};
    
    static const dict<data_type> type_dict{
        {"u8"s, data_type::U8},
        {"u16"s, data_type::U16},
        {"u32"s, data_type::U32},
        {"u64"s, data_type::U64},
        {"s8"s, data_type::S8},
        {"s16"s, data_type::S16},
        {"s32"s, data_type::S32},
        {"s64"s, data_type::S64},
        {"f32"s, data_type::F32},
        {"f64"s, data_type::F64}
    };
};

token compiler::next(bool nested) {
    if (done) {
        return token{token_type::END, token_end{}};
    }
    char c = stream.get();
    while (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == EOF) {
        file_pos++;
        if (c != '\n') {
            file_col++;
        } else {
            file_col = 1;
            file_line++;
        }
        if (c == EOF) {
            done = true;
            return token{token_type::END, token_end{}};
        }
        c = stream.get();
    }
    
    while (c == ';') { // Comment
        c = stream.get();
        while (c != EOF && c != '\n') {
            file_pos++;
            file_col++;
            c = stream.get();
        }
        while (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == EOF) {
            file_pos++;
            if (c != '\n') {
                file_col++;
            } else {
                file_col = 1;
                file_line++;
            }
            if (c == EOF) {
                done = true;
                return token{token_type::END, token_end{}};
            }
            c = stream.get();
        }
    }
    
    auto skip_whitespace = [this, &c](){
        while (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == EOF) {
            file_pos++;
            if (c != '\n') {
                file_col++;
            } else {
                file_col = 1;
                file_line++;
            }
            if (c == EOF) {
                done = true;
                error("Premature EOF");
                return false;
            }
            c = stream.get();
        }
        return true;
    };
    
    file_pos++;
    file_col++;
    
    // We are done with all the comments?
    
    std::string tok{c};
    
    if (c == '[') { // Memory location is special
        file_pos++;
        if(!skip_whitespace()) return token{token_type::ERROR, token_end{}};
        token pos = next(true);
        c = stream.peek();
        memory_variant pos_mv{};
        
        if (pos.is_register()) {
            pos_mv = pos.as_register();
        } else if (pos.is_immediate()) {
            pos_mv = pos.as_immediate();
        } else if (pos.is_iden()) {
            pos_mv = pos.as_iden();
        } else {
            error("Illegal memory position type");
        }
        if(!skip_whitespace()) return token{token_type::ERROR, token_end{}};
        if (c == ']') {
            stream.get();
            file_pos++;
            
            return token{token_type::MEMORY, token_memory{{}, pos.type, token_type::END, false, pos_mv, memory_variant{}}};
        } else if (c == '+' || c == '-') {
            bool negated = c == '-';
            if(!skip_whitespace()) return token{token_type::ERROR, token_end{}};
            token offs = next(true);
            c = stream.peek();
            memory_variant off_mv{};
            
            if (offs.is_register()) {
                off_mv = offs.as_register();
            } else if (offs.is_immediate()) {
                off_mv = offs.as_immediate();
            } else if (offs.is_iden()) {
                off_mv = offs.as_iden();
            } else {
                error("Illegal memory offset type");
            }
            if(!skip_whitespace()) return token{token_type::ERROR, token_end{}};
            if (c == ']') {
                stream.get();
                file_pos++;
                
                return token{token_type::MEMORY, token_memory{{}, pos.type, offs.type, negated, pos_mv, off_mv}};
            } else {
                logger::debug() << c << logger::nend;
                while (c != ']' && c != EOF) {
                    if (c != '\n') {
                        file_col++;
                    } else {
                        file_col = 1;
                        file_line++;
                    }
                    c = stream.get();
                }
                file_pos++;
                file_col++;
                error("Invalid end to memory location");
                return token{token_type::ERROR, token_end{}};
            }
        } else {
            while (c != ']' && c != EOF) {
                if (c != '\n') {
                    file_col++;
                } else {
                    file_col = 1;
                    file_line++;
                }
                tok += c;
                c = stream.get();
            }
            file_pos++;
            file_col++;
            error("Invalid end to memory location");
            return token{token_type::ERROR, token_end{}};
        }
    } else if (c == '"') { // String is also special
        c = stream.get();
        while (c != '"' && c != EOF) {
            file_pos++;
            if (c != '\n') {
                file_col++;
            } else {
                file_col = 1;
                file_line++;
            }
            if (c == '\\') {
                c = stream.get();
                switch (c) {
                    case 'n':
                        c = '\n';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                }
                file_pos++;
                if (c != '\n') {
                    file_col++;
                } else {
                    file_col = 1;
                    file_line++;
                }
            }
            tok += c;
            c = stream.get();
        }
        return token{token_type::STRING, token_string{tok}};
    } else {
        c = stream.peek();
        while (c != ';' && c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '+' && c != '-' && c != ']' && c != EOF) {
            tok += c;
            stream.get();
            file_pos++;
            if (c != '\n') {
                file_col++;
            } else {
                file_col = 1;
                file_line++;
            }
            c = stream.peek();
        }
    }
    
    // We got ourselves a token string
    
    if (tok[0] == '-' || tok[0] == '+' || std::isdigit(tok[0])) { // A number, weee
        bool sign = tok[0] == '-' || tok[0] == '+';
        if (sign && tok.length() < 2) {
            std::stringstream ss{};
            ss << tok << " is not a valid number";
            error(ss.str());
            return token{token_type::ERROR, token_end{}};
        }
        u8 base = 10;
        
        std::string_view signless = sign ? std::string_view{&tok[1], tok.length() - 1} : std::string_view{tok};
        
        if (signless.length() > 2 && signless[0] == '0') {
            if (signless[1] == 'x') {
                base = 16;
            } else if (signless[1] == 'o') {
                base = 8;
            } else if (signless[1] == 'b') {
                base = 2;
            }
        }
        
        if (sign && base != 10) {
            std::stringstream ss{};
            ss << "Cannot have both sign and alternative representation: " << tok;
            error(ss.str());
            return token{token_type::ERROR, token_end{}};
        }
        
        if (base != 10) {
            std::string_view letters{&signless[2], signless.length() - 2};
            char* after;
            u64 num = std::strtoull(letters.data(), &after, base);
            if (*after) {
                std::stringstream ss{};
                ss << "Found \"" << *after << "\" after valid number: " << tok;
                error(ss.str());
                return token{token_type::ERROR, token_end{}};
            }
            
            return token{token_type::IMMEDIATE, token_immediate{data_type::U64, num}}; // Note: Type is NOT made smaller by default. Never
        }
        
        bool floating = signless.find_first_of('.') != std::string::npos;
        if (floating) {
            char* after;
            double d = std::strtod(tok.data(), &after);
            if (*after) {
                std::stringstream ss{};
                ss << "Found \"" << *after << "\" after valid number: " << tok;
                error(ss.str());
                return token{token_type::ERROR, token_end{}};
            }
            u64 imm{};
            std::memcpy(&imm, &d, sizeof(double));
            return token{token_type::IMMEDIATE, token_immediate{data_type::F64, imm}};
        } else {
            if (tok[0] == '-') {
                char* after;
                s64 s = std::strtoll(tok.data(), &after, 10);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return token{token_type::ERROR, token_end{}};
                }
                u64 imm{0};
                std::memcpy(&imm, &s, sizeof(u64));
                return token{token_type::IMMEDIATE, token_immediate{data_type::S64, imm}};
            } else {
                char* after;
                u64 u = std::strtoll(tok.data(), &after, 10);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return token{token_type::ERROR, token_end{}};
                }
                return token{token_type::IMMEDIATE, token_immediate{data_type::U64, u}};
            }
        }
    } else if (tok[0] == '$') {
        if (tok.length() < 3 || tok.length() > 4) {
            std::stringstream ss{};
            ss << tok.substr(1) << " is not a valid register name";
            error(ss.str());
            return token{token_type::ERROR, token_end{}};
        }
        
        if (tok[1] == 'r' || tok[1] == 'f') {
            token_register reg{};
            if (tok[1] == 'f') {
                reg.floating = true;
                reg.type = data_type::F64;
            } else {
                reg.type = data_type::U64;
            }
            char* after;
            u32 n = std::strtoul(tok.data() + 2, &after, 10);
            if (*after) {
                std::stringstream ss{};
                ss << "Found \"" << *after << "\" after register name: " << tok;
                error(ss.str());
                return token{token_type::ERROR, token_end{}};
            }
            if (n == 0 || (reg.floating && n > 16) || (!reg.floating && n > 19)) {
                std::stringstream ss{};
                ss << tok.substr(1) << " is not a valid register name";
                error(ss.str());
                return token{token_type::ERROR, token_end{}};
            }
            reg.number = n;            
            return token{token_type::REGISTER, reg};
        } else if (tok.length() == 3) {
            if (tok[1] == 'p' && tok[2] == 'c') {
                return token{token_type::REGISTER, token_register{data_type::U64, false, 17}};
            } else if (tok[1] == 's' && tok[2] == 'f') {
                return token{token_type::REGISTER, token_register{data_type::U64, false, 18}};
            } else if (tok[1] == 's' && tok[2] == 'p') {
                return token{token_type::REGISTER, token_register{data_type::U64, false, 19}};
            }
        }
        
        std::stringstream ss{};
        ss << tok.substr(1) << " is not a valid register name";
        error(ss.str());
        return token{token_type::ERROR, token_end{}};
    } else if (tok[0] == '<') {
        auto ptr = values.find(tok.substr(1));
        if (ptr == values.end()) {
            std::stringstream ss{};
            ss << tok.substr(1) << " is not a valid value name";
            error(ss.str());
            return token{token_type::ERROR, token_end{}};
        } else {
            return ptr->second;
        }
    } else {
        if (auto ptr = name_to_op.find(tok); ptr != name_to_op.end()) {
            return token{token_type::OPCODE, token_opcode{ptr->second}};
        } else if (auto ptr = type_dict.find(tok); ptr != type_dict.end()) {
            token typ = token{token_type::TYPE, token_nnasm_type{ptr->second}};
            if (nested) {
                error("Type found when none expected");
            } else {
                token nret = next(true);
                switch (nret.type) {
                    case token_type::REGISTER:  nret.as_register().type  = typ.as_type().type; break;
                    case token_type::IMMEDIATE: nret.as_immediate().type = typ.as_type().type; break;
                    case token_type::MEMORY:    nret.as_memory().type    = typ.as_type().type; break;
                    default: error("Type applied to illegal token"); break;
                }
                return nret;
            }
        } else if (tok[0] == '~' || tok[0] == '_' || std::isalpha(tok[0])) {
            return token{token_type::IDEN, token_iden{tok}};
        } else {
            std::stringstream ss{};
            ss << "Invalid token \"" << tok << "\"";
            error(ss.str());
            return token{token_type::ERROR, token_end{}};
        }
    }
    
    return token{token_type::ERROR, token_end{}};
}

void compiler::error(const std::string& err) {
    std::stringstream ss{};
    ss << source << ":" << file_line << ":" << file_col << " - " << err;
    errors.push_back(ss.str());
}
