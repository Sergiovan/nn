#include "frontend/nnasm_compiler.h"

#include "common/convenience.h"
#include "common/utils.h"

#include <iomanip>
#include <filesystem>
#include <regex>
#include <sstream>

using namespace std::string_literals;
namespace fs = std::filesystem;

std::ostream& operator<<(std::ostream& os, nnasm_token_type t) {
    switch (t) {
        case nnasm_token_type::INVALID:
            return os << "Invalid";
        case nnasm_token_type::OPCODE:
            return os << "Opcode";
        case nnasm_token_type::REGISTER:
            return os << "Register";
        case nnasm_token_type::MEMORY:
            return os << "Memory";
        case nnasm_token_type::IMMEDIATE:
            return os << "Immediate";
        case nnasm_token_type::INDIRECT:
            return os << "Indirect";
        case nnasm_token_type::VALUE:
            return os << "Value";
        case nnasm_token_type::STRING:
            return os << "String";
        case nnasm_token_type::IDEN:
            return os << "Iden";
        case nnasm_token_type::TYPE:
            return os << "Type";
        case nnasm_token_type::END:
            return os << "End";
        case nnasm_token_type::ERROR:
            return os << "Error";
        default:
            return os << "???";
    }
}

std::ostream& operator<<(std::ostream& os, nnasm_type t) {
    switch (t) {
        case nnasm_type::U8:
            return os << "u8";
        case nnasm_type::U16:
            return os << "u16";
        case nnasm_type::U32:
            return os << "u32";
        case nnasm_type::U64:
            return os << "u64";
        case nnasm_type::S8:
            return os << "s8";
        case nnasm_type::S16:
            return os << "s16";
        case nnasm_type::S32:
            return os << "s32";
        case nnasm_type::S64:
            return os << "s64";
        case nnasm_type::F32:
            return os << "f32";
        case nnasm_type::F64:
            return os << "f64";
        case nnasm_type::NONE:
            return os << "u64";
        default:
            return os << "???";
    }
}

bool nnasm_token::is_opcode() {
    return type == nnasm_token_type::OPCODE;
}

bool nnasm_token::is_register() {
    return type == nnasm_token_type::REGISTER;
}

bool nnasm_token::is_memory() {
    return type == nnasm_token_type::MEMORY;
}

bool nnasm_token::is_immediate() {
    return type == nnasm_token_type::IMMEDIATE;
}

bool nnasm_token::is_indirect() {
    return type == nnasm_token_type::INDIRECT;
}

bool nnasm_token::is_string() {
    return type == nnasm_token_type::STRING;
}

bool nnasm_token::is_iden() {
    return type == nnasm_token_type::IDEN;
}

bool nnasm_token::is_type() {
    return type == nnasm_token_type::TYPE;
}

bool nnasm_token::is_end() {
    return type == nnasm_token_type::END;
}

bool nnasm_token::is_error() {
    return type == nnasm_token_type::ERROR;
}

nnasm_token_opcode& nnasm_token::as_opcode() {
    return std::get<nnasm_token_opcode>(data);
}

nnasm_token_register& nnasm_token::as_register() {
    return std::get<nnasm_token_register>(data);
}

nnasm_token_memory& nnasm_token::as_memory() {
    return std::get<nnasm_token_memory>(data);
}

nnasm_token_immediate& nnasm_token::as_immediate() {
    return std::get<nnasm_token_immediate>(data);
}

nnasm_token_indirect& nnasm_token::as_indirect() {
    return std::get<nnasm_token_indirect>(data);
}

nnasm_token_string& nnasm_token::as_string() {
    return std::get<nnasm_token_string>(data);
}

nnasm_token_iden& nnasm_token::as_iden() {
    return std::get<nnasm_token_iden>(data);
}

nnasm_token_nnasm_type& nnasm_token::as_type() {
    return std::get<nnasm_token_nnasm_type>(data);
}

nnasm_token_end& nnasm_token::as_end() {
    return std::get<nnasm_token_end>(data);
}

std::string nnasm_token::print() {
    std::stringstream ss{};
    ss << type << ": ";
    switch (type) {
        case nnasm_token_type::INVALID:
            ss << "?????";
            break;
        case nnasm_token_type::OPCODE:
            ss << nnasm::op_to_name.at(as_opcode().opcode);
            break;
        case nnasm_token_type::REGISTER: {
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
        case nnasm_token_type::MEMORY: {
            auto& mem = as_memory();
            ss << mem.type << " [";
            if (mem.data == nnasm_token_type::REGISTER) {
                ss << nnasm_token{mem.data, mem.reg_data}.print();
            } else if (mem.data == nnasm_token_type::IMMEDIATE) {
                ss << mem.imm_data;
            } else {
                ss << nnasm_token{mem.data, mem.ind_data}.print();
            }
            
            if (mem.offset != nnasm_token_type::END) {
                if (mem.offset_signed) {
                    ss << " - ";
                } else {
                    ss << " + ";
                }
                
                if (mem.offset == nnasm_token_type::REGISTER) {
                    ss << nnasm_token{mem.offset, mem.reg_offset}.print();
                } else if (mem.offset == nnasm_token_type::IMMEDIATE) {
                    ss << mem.imm_offset;
                } else {
                    ss << nnasm_token{mem.offset, mem.ind_offset}.print();
                }
            }
            ss << "]";
            break;
        }
        case nnasm_token_type::IMMEDIATE:
            ss << as_immediate().type << " " << as_immediate().data;
            break;
        case nnasm_token_type::INDIRECT:
            ss << *as_indirect().tok; // TODO
            break;
        case nnasm_token_type::VALUE:
            ss << "?????";
            break;
        case nnasm_token_type::STRING:
            ss << as_string().str;
            break;
        case nnasm_token_type::IDEN:
            ss << as_iden().iden;
            break;
        case nnasm_token_type::TYPE:
            ss << as_type().type;
            break;
        case nnasm_token_type::END:
            ss << "EOF";
            break;
        case nnasm_token_type::ERROR:
            break;
        default:
            ss << "???";
            break;
    }
    
    return ss.str();
}

nnasm_compiler::nnasm_compiler(const std::string& file) {
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

nnasm_compiler::~nnasm_compiler() {
    if (program) {
        delete [] program;
    }
    
    for (auto& token : tokens) {
        if (token) {
            delete token;
        }
    }
    
    for (auto& [name, ind] : indirects) {
        if (ind) {
            delete ind;
        }
    }
}

void nnasm_compiler::compile() {
    if (done) {
        return;
    }
    first_pass();
    second_pass();
    
    for (auto& tok : tokens) {
        logger::debug() << tok->print() << logger::nend;
    } 
    
}

void nnasm_compiler::first_pass() {
    if (done) {
        return;
    }
    
    auto expect = [this] (nnasm_token& tok, nnasm_token_type type) {
        if (tok.type != type) {
            std::stringstream ss{};
            ss << "Expected " << type << " but got \"" << tok.print() << "\" instead";
            error(ss.str());
            return false;
        }
        return true;
    };
    
    using namespace nnasm;
    
    auto fmts = format::get_formats();
    
    auto cur = next();
    u64 i = 1;
    while (!cur.is_end()) {
        if (!expect(cur, nnasm_token_type::OPCODE)) {
            cur = next();
            continue;
        }
        
        switch (cur.as_opcode().opcode) {
            case opcode::LBL: {
                auto idn = next();
                if (!expect(idn, nnasm_token_type::IDEN)) {
                    cur = next();
                    continue;
                }
                tokens.push_back(new nnasm_token{cur});
                tokens.push_back(new nnasm_token{idn});
                auto& iden = idn.as_iden().iden;
                if (auto ptr = indirects.find(iden); ptr == indirects.end()) {
                    indirects.insert({idn.as_iden().iden, new u64{tokens.size() - 1}});
                } else {
                    *(ptr->second) = tokens.size() - 1;
                }
                cur = next();
                continue;
            }
            case opcode::VAL: {
                auto idn = next(); 
                if (!expect(idn, nnasm_token_type::IDEN)) {
                    cur = next();
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
                cur = next();
                continue;
            }
            case opcode::DB: {
                auto idn = next();
                if (!expect(idn, nnasm_token_type::IDEN)) {
                    cur = next();
                    continue;
                }
                tokens.push_back(new nnasm_token{cur});
                tokens.push_back(new nnasm_token{idn});
                auto& iden = idn.as_iden().iden;
                if (auto ptr = indirects.find(iden); ptr == indirects.end()) {
                    indirects.insert({idn.as_iden().iden, new u64{tokens.size() - 1}});
                } else {
                    *(ptr->second) = tokens.size() - 1;
                }
                cur = next();
                while (!cur.is_end() && !cur.is_opcode()) {
                    if (cur.is_error()) {
                        prev_type = nnasm_type::NONE;
                        cur = next();
                        continue;
                    }
                    if (cur.is_type()) {
                        prev_type = cur.as_type().type;
                        cur = next();
                        continue;
                    }
                    if (!expect(cur, nnasm_token_type::IMMEDIATE)) {
                        cur = next();
                        continue;
                    }
                    tokens.push_back(new nnasm_token{cur});
                    prev_type = nnasm_type::NONE;
                    cur = next();
                }
                continue;
            }
            case opcode::DBS: { // TODO
                auto idn = next();
                if (!expect(idn, nnasm_token_type::IDEN)) {
                    cur = next();
                    continue;
                }
                tokens.push_back(new nnasm_token{cur});
                tokens.push_back(new nnasm_token{idn});
                auto& iden = idn.as_iden().iden;
                if (auto ptr = indirects.find(iden); ptr == indirects.end()) {
                    indirects.insert({idn.as_iden().iden, new u64{tokens.size() - 1}});
                } else {
                    *(ptr->second) = tokens.size() - 1;
                }
                cur = next();
                while (!cur.is_end() && !cur.is_opcode()) {
                    if (cur.is_error()) {
                        prev_type = nnasm_type::NONE;
                        cur = next();
                        continue;
                    }
                    if (cur.is_type()) {
                        prev_type = cur.as_type().type;
                        cur = next();
                        continue;
                    }
                    if (!expect(cur, nnasm_token_type::IMMEDIATE)) {
                        cur = next();
                        continue;
                    }
                    tokens.push_back(new nnasm_token{cur});
                    prev_type = nnasm_type::NONE;
                    cur = next();
                }
                continue;
            }
            default:
                break;
        }
        
        tokens.push_back(new nnasm_token{cur});
        
        opcode code = cur.as_opcode().opcode;
        format::instruction instr;
        u8 op = 0;
        cur = next();
        while (!cur.is_end() && !cur.is_opcode()) {
            if (cur.is_error()) {
                prev_type = nnasm_type::NONE;
                cur = next();
                continue;
            }
            if (cur.is_type()) {
                prev_type = cur.as_type().type;
                cur = next();
                continue;
            }
            
            if (cur.is_immediate()) {
                instr.op[op].imm = true;
                prev_type = cur.as_immediate().type;
            } else if (cur.is_register()) {
                instr.op[op].reg = true;
                prev_type = cur.as_register().type;
            } else if (cur.is_memory()) {
                instr.op[op].mem = true;
                prev_type = cur.as_memory().type;
            } else if (cur.is_iden()) {
                instr.op[op].imm = true;
                if (prev_type == nnasm_type::NONE) {
                    prev_type = nnasm_type::U64;
                }
            } else if (cur.is_indirect() || cur.is_string()) {
                std::stringstream ss{};
                ss << "Invalid token " << cur.print();
                error(ss.str());
                prev_type = nnasm_type::U64;
            }

            tokens.push_back(new nnasm_token{cur});
            
            switch (prev_type) {
                case nnasm_type::U8:
                    instr.op[op].u = true;
                    instr.op[op]._8 = true;
                    break;
                case nnasm_type::U16:
                    instr.op[op].u = true;
                    instr.op[op]._16 = true;
                    break;
                case nnasm_type::U32:
                    instr.op[op].u = true;
                    instr.op[op]._32 = true;
                    break;
                case nnasm_type::U64:
                    instr.op[op].u = true;
                    instr.op[op]._64 = true;
                    break;
                case nnasm_type::S8:
                    instr.op[op].s = true;
                    instr.op[op]._8 = true;
                    break;
                case nnasm_type::S16:
                    instr.op[op].s = true;
                    instr.op[op]._16 = true;
                    break;
                case nnasm_type::S32:
                    instr.op[op].s = true;
                    instr.op[op]._32 = true;
                    break;
                case nnasm_type::S64:
                    instr.op[op].s = true;
                    instr.op[op]._64 = true;
                    break;
                case nnasm_type::F32:
                    instr.op[op].f = true;
                    instr.op[op]._32 = true;
                    break;
                case nnasm_type::F64:
                    instr.op[op].f = true;
                    instr.op[op]._64 = true;
                    break;
                case nnasm_type::NONE: {
                    std::stringstream ss{};
                    ss << "Type for " << cur.print() << " not defined";
                    error(ss.str());
                    break;
                }
            }
            
            ++op;
            
            prev_type = nnasm_type::NONE;
            cur = next();
        }
        
        auto& formats = fmts.at(code);
        bool found = false;
        for (auto& format : formats) {
            if ((format.raw & instr.raw) == instr.raw) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            std::stringstream ss{};
            ss << "Invalid format for " << code << ": 0x" << std::hex << instr.raw;
            error(ss.str());
        }
    }
}

void nnasm_compiler::second_pass() {
    if (done) {
        return;
    }
}

void nnasm_compiler::print_errors() {
    for (auto& err : errors) {
        logger::error() << err << logger::nend;
    }
}

u8* nnasm_compiler::get_program() {
    return program;
}

u8* nnasm_compiler::move_program() {
    u8* buff = program;
    program = nullptr;
    return buff;
}

u64 nnasm_compiler::get_size() {
    return program_size;
}

void nnasm_compiler::store_to_file(const std::string& filename) {
    
}

namespace {
    static const std::regex memory_location_rgx{"^\\[ *([^ ]+) *(?:(\\+|-) *([^ ]+) *)?\\]$", std::regex::optimize};
    
    static const dict<nnasm_type> type_dict{
        {"u8"s, nnasm_type::U8},
        {"u16"s, nnasm_type::U16},
        {"u32"s, nnasm_type::U32},
        {"u64"s, nnasm_type::U64},
        {"s8"s, nnasm_type::S8},
        {"s16"s, nnasm_type::S16},
        {"s32"s, nnasm_type::S32},
        {"s64"s, nnasm_type::S64},
        {"f32"s, nnasm_type::F32},
        {"f64"s, nnasm_type::F64}
    };
};

nnasm_token nnasm_compiler::next() {
    if (done) {
        return nnasm_token{nnasm_token_type::END, nnasm_token_end{}};
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
            return nnasm_token{nnasm_token_type::END, nnasm_token_end{}};
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
                return nnasm_token{nnasm_token_type::END, nnasm_token_end{}};
            }
            c = stream.get();
        }
    }
    
    file_pos++;
    file_col++;
    
    // We are done with all the comments?
    
    std::string tok{c};
    
    if (c == '[') { // Memory location is special
        c = stream.get();
        while (c != ']' && c != EOF) {
            file_pos++;
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
        if (c != EOF) {
            tok += c;
            c = stream.get(); // ]
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
    } else {
        c = stream.peek();
        while (c != ';' && c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != EOF) {
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
    
    return to_token(tok);
}

nnasm_token nnasm_compiler::to_token(const std::string& tok) {
    using namespace nnasm;
    
    if (tok[0] == '[') {
        std::smatch matches;
        if (std::regex_search(tok, matches, memory_location_rgx)) {
            nnasm_token_memory mem{};
            
            mem.offset = nnasm_token_type::END;
            
            if (prev_type == nnasm_type::NONE) {
                prev_type = nnasm_type::U64;
            }
            
            mem.type = prev_type;
            
            nnasm_token first = to_token(matches[1].str());
            if (!first.is_register() && !first.is_immediate() && !first.is_iden()) {
                std::stringstream ss{};
                ss << matches[1].str() << " is not a valid memory location target";
                error(ss.str());
                return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
            }
            
            mem.data = first.type;
            if (mem.data == nnasm_token_type::IMMEDIATE) {
                mem.imm_data = first.as_immediate().data;
            } else if (mem.data == nnasm_token_type::REGISTER) {
                mem.reg_data = first.as_register();
            } else {
                mem.data = nnasm_token_type::INDIRECT;
                auto ptr = indirects.find(first.as_iden().iden);
                if (ptr == indirects.end()) {
                    ptr = indirects.insert({first.as_iden().iden, new u64{0}}).first;
                }
                
                mem.ind_data = nnasm_token_indirect{ptr->second};
            }
            
            if (matches[2].matched) {
                if (matches[2].str()[0] == '-') {
                    mem.offset_signed = true;
                    prev_type = nnasm_type::U32;
                }
                
                nnasm_token second = to_token(matches[3].str());
                if (!second.is_register() && !second.is_immediate() && !second.is_iden()) {
                    std::stringstream ss{};
                    ss << matches[3].str() << " is not a valid memory location offset";
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                
                mem.offset = second.type;
                if (mem.offset == nnasm_token_type::IMMEDIATE) {
                    mem.imm_offset = second.as_immediate().data;
                } else if (mem.offset == nnasm_token_type::REGISTER) {
                    mem.reg_offset = second.as_register();
                } else {
                    mem.offset = nnasm_token_type::INDIRECT;
                    auto ptr = indirects.find(second.as_iden().iden);
                    if (ptr == indirects.end()) {
                        ptr = indirects.insert({second.as_iden().iden, new u64{0}}).first;
                    }
                    
                    mem.ind_offset = nnasm_token_indirect{ptr->second};
                }
            }
            
            return nnasm_token{nnasm_token_type::MEMORY, mem};
            
        } else {
            std::stringstream ss{};
            ss << tok.substr(1) << " is not a valid memory location";
            error(ss.str());
            return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
        }
    } /*else if (tok[0] == '~') {
    
    }*/ else if (tok[0] == '$') {
        if (tok.length() < 3 || tok.length() > 4) {
            std::stringstream ss{};
            ss << tok.substr(1) << " is not a valid register name";
            error(ss.str());
            return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
        }
        
        if (tok[1] == 'r' || tok[1] == 'f') {
            nnasm_token_register reg{};
            if (tok[1] == 'f') {
                reg.floating = true;
                if (prev_type == nnasm_type::NONE) {
                    prev_type = nnasm_type::F64;
                }
            } else if (prev_type == nnasm_type::NONE) {
                prev_type = nnasm_type::U64;
            }
            reg.type = prev_type;
            char* after;
            u32 n = std::strtoul(tok.data() + 2, &after, 10);
            if (*after) {
                std::stringstream ss{};
                ss << "Found \"" << *after << "\" after register name: " << tok;
                error(ss.str());
                return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
            }
            if (n == 0 || (reg.floating && n > 16) || (!reg.floating && n > 19)) {
                std::stringstream ss{};
                ss << tok.substr(1) << " is not a valid register name";
                error(ss.str());
                return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
            }
            reg.number = n;            
            return nnasm_token{nnasm_token_type::REGISTER, reg};
        } else if (tok.length() == 3) {
            if (tok[1] == 'p' && tok[2] == 'c') {
                return nnasm_token{nnasm_token_type::REGISTER, nnasm_token_register{prev_type, false, 17}};
            } else if (tok[1] == 's' && tok[2] == 'f') {
                return nnasm_token{nnasm_token_type::REGISTER, nnasm_token_register{prev_type, false, 18}};
            } else if (tok[1] == 's' && tok[2] == 'p') {
                return nnasm_token{nnasm_token_type::REGISTER, nnasm_token_register{prev_type, false, 19}};
            }
        }
        
        std::stringstream ss{};
        ss << tok.substr(1) << " is not a valid register name";
        error(ss.str());
        return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
    } else if (tok[0] == '<') {
        auto ptr = values.find(tok.substr(1));
        if (ptr == values.end()) {
            std::stringstream ss{};
            ss << tok.substr(1) << " is not a valid value name";
            error(ss.str());
            return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
        } else {
            return ptr->second;
        }
    } else if (std::isdigit(tok[0]) || tok[0] == '-') {
        bool sign = tok[0] == '-';
        if (sign && tok.length() < 2) {
            std::stringstream ss{};
            ss << tok << " is not a valid number";
            error(ss.str());
            return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
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
            ss << "Cannot have both - and alternative representation: " << tok;
            error(ss.str());
            return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
        }
        
        if (base != 10) {
            std::string_view letters{&signless[2], signless.length() - 2};
            char* after;
            u64 num = std::strtoull(letters.data(), &after, base);
            if (*after) {
                std::stringstream ss{};
                ss << "Found \"" << *after << "\" after valid number: " << tok;
                error(ss.str());
                return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
            }

            return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, num}};
        }
        
        bool floating = signless.find_first_of('.') != std::string::npos;
        if (floating) {
            if (prev_type == nnasm_type::F32) {
                char* after;
                float f = std::strtof(tok.data(), &after);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                u32 imm{};
                std::memcpy(&imm, &f, sizeof(u32));
                return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, imm}};
            } else if (prev_type == nnasm_type::F64 || prev_type == nnasm_type::NONE) {
                char* after;
                float d = std::strtod(tok.data(), &after);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                prev_type = nnasm_type::F64;
                u64 imm{};
                std::memcpy(&imm, &d, sizeof(u64));
                return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, imm}};
            } else {
                std::stringstream ss{};
                ss << "Cannot convert \"" << tok << "\" to " << prev_type;
                error(ss.str());
                return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
            }
        } else {
            if (prev_type == nnasm_type::S8 || prev_type == nnasm_type::S16 || prev_type == nnasm_type::S32 || prev_type == nnasm_type::S64 || (sign && prev_type == nnasm_type::NONE)) {
                char* after;
                i64 s = std::strtoll(tok.data(), &after, 10);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                if (prev_type == nnasm_type::NONE) {
                    if (s == (i8) s) {
                        prev_type = nnasm_type::S8;
                    } else if (s == (i16) s) {
                        prev_type = nnasm_type::S16;
                    } else if (s == (i32) s) {
                        prev_type = nnasm_type::S32;
                    } else {
                        prev_type = nnasm_type::S64;
                    }
                } else if ((prev_type == nnasm_type::S8 && s != (i8) s) || (prev_type == nnasm_type::S16 && s != (i16) s) || (prev_type == nnasm_type::S32 && s != (i32) s)) {
                    std::stringstream ss{};
                    ss << "Cannot convert \"" << tok << "\" to " << prev_type;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}}; 
                }
                u64 imm{0};
                std::memcpy(&imm, &s, sizeof(u64));
                return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, imm}};
            } else if (prev_type == nnasm_type::F32) {
                char* after;
                float f = std::strtof(tok.data(), &after);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                u32 imm{};
                std::memcpy(&imm, &f, sizeof(u32));
                return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, imm}};
            } else if (prev_type == nnasm_type::F64) {
                char* after;
                double d = std::strtod(tok.data(), &after);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                u64 imm{};
                std::memcpy(&imm, &d, sizeof(u64));
                return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, imm}};
            } else {
                char* after;
                u64 u = std::strtoll(tok.data(), &after, 10);
                if (*after) {
                    std::stringstream ss{};
                    ss << "Found \"" << *after << "\" after valid number: " << tok;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                if (prev_type == nnasm_type::NONE) {
                    if (u == (u8) u) {
                        prev_type = nnasm_type::U8;
                    } else if (u == (u16) u) {
                        prev_type = nnasm_type::U16;
                    } else if (u == (u32) u) {
                        prev_type = nnasm_type::U32;
                    } else {
                        prev_type = nnasm_type::U64;
                    }
                } else if ((prev_type == nnasm_type::U8 && u != (u8) u) || (prev_type == nnasm_type::U16 && u != (u16) u) || (prev_type == nnasm_type::U32 && u != (u32) u)) {
                    std::stringstream ss{};
                    ss << "Cannot convert \"" << tok << "\" to " << prev_type;
                    error(ss.str());
                    return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
                }
                return nnasm_token{nnasm_token_type::IMMEDIATE, nnasm_token_immediate{prev_type, u}};
            }
        }
    } else if (tok[0] == '"') {
        return nnasm_token{nnasm_token_type::STRING, nnasm_token_string{tok.substr(1, tok.length() - 2)}};
    } else if (auto ptr = name_to_op.find(tok); ptr != name_to_op.end()) {
        return nnasm_token{nnasm_token_type::OPCODE, nnasm_token_opcode{ptr->second}};
    } else if (auto ptr = type_dict.find(tok); ptr != type_dict.end()) {
        return nnasm_token{nnasm_token_type::TYPE, nnasm_token_nnasm_type{ptr->second}};
    } else if (tok[0] == '_' || std::isalpha(tok[0])) {
        prev_type = nnasm_type::U64;
        return nnasm_token{nnasm_token_type::IDEN, nnasm_token_iden{tok}};
    } else {
        std::stringstream ss{};
        ss << "Invalid token \"" << tok << "\"";
        error(ss.str());
        return nnasm_token{nnasm_token_type::ERROR, nnasm_token_end{}};
    }
}


void nnasm_compiler::error(const std::string& err) {
    std::stringstream ss{};
    ss << source << ":" << file_line << ":" << file_col << " - " << err;
    errors.push_back(ss.str());
}
