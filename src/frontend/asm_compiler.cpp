#include "frontend/asm_compiler.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include "common/utils.h" 
#include "common/type.h"
#include "vm/machine.h"


asm_compiler::asm_compiler(const std::string& filename) : filename(filename), file(filename, std::ios_base::in) {
    if (!file || !file.good()) {
        logger::error() << "Could not open file " << filename << logger::nend;
        // TODO why tho
    }
    
    ready = true;
}

asm_compiler::~asm_compiler() {
    delete [] buffer;
    buffer = nullptr;
    if (compiled) {
        delete compiled;
        compiled = nullptr;
    }
    
    compiled_size = 0;
    
    for (auto& [name, value] : database) {
        if (value.data) {
            delete [] value.data;
        }
    }
}

bool conforms(nnasm::format::operand_format format, nnasm::instruction_code::operand op) {
    using namespace nnasm::op;
    if (op.optype == ASM_TYPE_INVALID) {
        return false;
    }
    
    if (format == nnasm::format::OP_NONE && op.optype) {
        return false;
    }
    
    if (format != nnasm::format::OP_NONE && !op.optype) {
        return false;
    }
    
    if (((format & nnasm::format::OP_TYPE) & op.optype) != op.optype) {
        return false;
    }
    
    if (op.optype == OP_NONE) {
        return true;
    }
    
    if (!(format & nnasm::format::OP_FLAGS)) {
        return true;
    }
    
    u8 type = op.deref ? op.deref : op.valtype;
    
    if ((format & nnasm::format::OP_FLAGS_FLOAT) && (type & ASM_TYPE_FLOATING)) {
        return true;
    }
    
    if ((format & nnasm::format::OP_FLAGS_SINT) && (type & ASM_TYPE_SIGNED)) {
        return true;
    }
    
    if ((format & nnasm::format::OP_FLAGS_UINT) && !(type & ASM_TYPE_SIGNED)) {
        return true;
    }
    
    return false;
}

void asm_compiler::compile() {
    using namespace std::string_literals;
    
    if (!ready) {
        errors.push_back(filename + ":1 - Nothing to compile.");
        return;
    }
    
    nnexe_header hdr;
    
    compiled = new u8[1024];
    compiled_size = 1024;
    std::memset(compiled, 0, compiled_size);
    
    shaddr = addr = hdr.code_start; // Programs start at address 128
    
    nnasm::instruction_code hlt{0};
    hlt.opcode = nnasm::op::HLT;
    
    while (!file.eof()) {
        using namespace nnasm;
        using namespace op;
        
        instruction ins = next_instruction();
        
        switch (ins.code.opcode) {
            case INVALID_CODE:
                add(hlt);
                shaddr = addr;
                continue;
            case VAL: {
                std::string idn{};
                while (!peek_separator() && !peek_eol()) {
                    idn += next_char();
                }
                next_char(); // Skip, skip, SKIP
                std::string valstr{};
                next_value(ins, 0, &valstr, false);
                next_char();
                
                if (std::toupper(idn[0]) < 'A' || std::toupper(idn[0]) > 'Z') {
                    std::stringstream ss{};
                    ss << "Invalid start of identifier '" << idn[0] << "'";
                    error(ss.str());
                    continue;
                }
                
                if (auto it = literals.find(idn); it != literals.end()) {
                    std::stringstream ss{};
                    ss << "Literal with name \"" << idn << "\" already exists, with value \"" << it->second << "\"";
                    error(ss.str());
                    continue;
                }
                
                if (ins.code.ops[0].valtype != ASM_TYPE_INVALID) {
                    literals.insert({idn, valstr});
                } else {
                    literals.insert({idn, "0"s});
                }
                continue;
            }
            case LBL: {
                std::string idn{};
                while (!peek_separator() && !peek_eol()) {
                    idn += next_char();
                }
                next_char(); // Skip, skip, SKIP
                
                if (std::toupper(idn[0]) < 'A' || std::toupper(idn[0]) > 'Z') {
                    std::stringstream ss{};
                    ss << "Invalid start of identifier '" << idn[0] << "'";
                    error(ss.str());
                    continue;
                }
                
                if (auto it = labels.find(idn); it != labels.end()) {
                    std::stringstream ss{};
                    ss << "Label with name \"" << idn << "\" already exists, with value \"" << std::hex << it->second << "\"";
                    error(ss.str());
                    continue;
                }
                
                labels.insert({idn, addr});
                continue;
            }
            case DB: [[fallthrough]];
            case DBS: {
                vmregister converter{};
                bool nnstr = false;
                if (ins.code.opcode == DBS) {
                    nnstr = true;
                }
                
                std::string idn{};
                while (!peek_separator() && !peek_eol()) {
                    idn += next_char();
                }
                next_char(); // Skip, skip, SKIP
                
                if (std::toupper(idn[0]) < 'A' || std::toupper(idn[0]) > 'Z') {
                    std::stringstream ss{};
                    ss << "Invalid start of identifier '" << idn[0] << "'";
                    error(ss.str());
                    
                    char s = next_char();
                    while (!file.eof() && s != '"') {
                        s = next_char();
                    }
                    
                    continue;
                }
                
                if (auto it = labels.find(idn); it != labels.end()) {
                    std::stringstream ss{};
                    ss << "Label with name \"" << idn << "\" already exists, with value \"" << std::hex << it->second << "\"";
                    error(ss.str());
                    
                    char s = next_char();
                    while (!file.eof() && s != '"') {
                        s = next_char();
                    }
                    
                    continue;
                }
                
                char c = ' ';
                db_value dbval;
                dbval.data = new u8[64];
                dbval.size = 64;
                dbval.addr = nnstr ? 16 : 0;
                
                if (nnstr) {
                    u64 strtype = etype_ids::STRING;
                    std::memcpy(dbval.data, &strtype, sizeof(strtype));
                }
                
                while (c != '\n') {
                    if (file.peek() == '"') {
                        char s = next_char();
                        std::string val{};
                        while (!file.eof()) {
                            s = next_char();
                            if (s == '\\') {
                                s = next_char();
                                switch (s) {
                                    case 'n':
                                        val += '\n';
                                        break;
                                    case 't':
                                        val += '\t';
                                        break;
                                    case '0':
                                        val += '\0';
                                        break;
                                    case '\r':
                                        if (file.peek() != '\n') {
                                            val += '\r';
                                            break;
                                        }
                                        s = next_char();
                                        [[fallthrough]];
                                    case '\n':
                                        val += ' ';
                                        break;
                                    default:
                                        val += s;
                                        break;
                                }
                            } else if (s == '"') {
                                break;
                            } else if (s == '\n') {
                                std::stringstream ss{};
                                ss << "String has unescaped newline";
                                error(ss.str());
                                
                                while (!file.eof() && s != '"') {
                                    s = next_char();
                                }
                                
                                break;
                            } else {
                                val += s;
                            }
                        }
                        
                        for (u64 i = 0; i < val.length();) {
                            u8 charlen;
                            if (nnstr) {
                                charlen = utils::utflen(val[i]);
                            } else {
                                charlen = 1;
                            }
                            if (dbval.addr + charlen > dbval.size) {
                                u8* buff = new u8[dbval.size * 2];
                                std::memcpy(buff, dbval.data, dbval.size);
                                delete [] dbval.data;
                                dbval.data = buff;
                                dbval.size *= 2;
                            }
                            std::memcpy(dbval.data + dbval.addr, &val[i], charlen);
                            dbval.addr += charlen;
                            i += charlen;
                        }
                        
                    } else {
                        u64 val = next_value(ins, 0, nullptr, false, false);
                        u8 valsize;
                        switch (ins.code.ops[0].valtype) {
                            case ASM_TYPE_BYTE: [[fallthrough]];
                            case ASM_TYPE_SBYTE:
                                valsize = 1;
                                break;
                            case ASM_TYPE_WORD: [[fallthrough]];
                            case ASM_TYPE_SWORD:
                                valsize = 2;
                                break;
                            case ASM_TYPE_DWORD: [[fallthrough]];
                            case ASM_TYPE_SDWORD: [[fallthrough]];
                            case ASM_TYPE_FLOAT:
                                valsize = 4;
                                break;
                            case ASM_TYPE_QWORD: [[fallthrough]];
                            case ASM_TYPE_SQWORD: [[fallthrough]];
                            case ASM_TYPE_DOUBLE: 
                                valsize = 8;
                                break;
                        }
                        if (nnstr) {
                            valsize = std::min((u8) 4, valsize);
                        }
                        if (dbval.addr + valsize > dbval.size) {
                            u8* buff = new u8[dbval.size * 2];
                            std::memcpy(buff, dbval.data, dbval.size);
                            delete [] dbval.data;
                            dbval.data = buff;
                            dbval.size *= 2;
                        }
                        std::memcpy(dbval.data + dbval.addr, &val, valsize);
                        dbval.addr += valsize;
                    }
                    c = next_char();
                }
                
                if (nnstr) {
                    u64 len = (dbval.addr - 8) / 8;
                    std::memcpy(dbval.data + 8, &len, sizeof(len));
                }
                
                dbval.size = dbval.addr;
                dbval.addr = 0;
                
                database.insert({idn, dbval});
                
                continue;
            }
        }
        
        auto& formats = nnasm::format::instructions.at((nnasm::op::code) ins.code.opcode);
        u8 conforms_to{0};
        for (auto& format : formats) {
            if (conforms(format.op1, ins.code.ops[0]) && conforms(format.op2, ins.code.ops[1]) && conforms(format.op3, ins.code.ops[2])) {
                ++conforms_to;
            }
        }
        
        if (conforms_to != 1) {
            std::stringstream ss{};
            ss << "Invalid operands for opcode";
            error(ss.str());
            shaddr = addr;
            continue;
        }
        
        add(reinterpret_cast<u8*>(&ins.code), sizeof(ins.code));
        for (u8 i = 0; i < 3; ++i) {
            if (ins.code.ops[i].optype == OP_NONE) {
                break;
            } else if (ins.code.ops[i].optype == OP_REG) {
                continue;
            }
            add(reinterpret_cast<u8*>(&ins.values[i]), sizeof(ins.values[i]));
        }
    }
    
    hdr.data_start = addr;
    
    if (!database.empty()) {
        for (auto& [name, value] : database) {
            value.addr = addr;
            add(value.data, value.size);
            delete [] value.data;
            value.data = nullptr;
            labels.insert({"&"s + name, value.addr});
            
            u8 padding = ((8 - (value.size % 8)) % 8);
            u64 zero = 0ull;
            add(reinterpret_cast<u8*>(&zero), padding);
        }
    }
    
    hdr.size = addr;
    
    if (!pending.empty()) {
        for (auto& [addr, name] : pending) {
            auto it = labels.find(name);
            if (it == labels.end()) {
                std::stringstream ss{};
                ss << "\"" << name << "\" does not exist";
                error(ss.str());
                continue;
            } else {
                std::memcpy(compiled + addr, &it->second, sizeof(it->second));
            }
        }
    }
    
    std::memcpy(compiled, &hdr, sizeof(hdr));
    
    u8 padding = ((8 - (hdr.size % 8)) % 8);
    u8* buff = new u8[hdr.size + padding];
    std::memcpy(buff, compiled, hdr.size + padding);
    delete [] buff;
    compiled_size = hdr.size + padding;
    
    if (!errors.empty()) {
        print_errors();
    }
}

void asm_compiler::print_errors() {
    for (auto& error : errors) {
        logger::error() << error << logger::nend;
    }
}

u8* asm_compiler::get() {
    return compiled;
}

u8* asm_compiler::move() {
    u8* buff = compiled;
    compiled = nullptr;
    compiled_size = 0;
    return buff;
}

u64 asm_compiler::size() {
    return compiled_size;
}

void asm_compiler::store_to_file(const std::string& filename) {

}

void asm_compiler::add(u8* data, u64 length) {
    if (addr + length > compiled_size) {
        u8* buff = new u8[compiled_size * 2];
        std::memcpy(buff, compiled, compiled_size);
        compiled_size = compiled_size * 2;
        delete [] compiled;
        compiled = buff;
    }
    
    std::memcpy(compiled + addr, data, length);
    addr += length;
}

nnasm::instruction asm_compiler::next_instruction() {
    using namespace nnasm::op;
    
    nnasm::instruction ins{0};
    
    while ((peek_separator() || peek_eol()) && !file.eof()) {
        next_char();
    }
    
    char n;
    ins.code.opcode = next_opcode();
    n = next_char(); // SKIP
    
    switch (ins.code.opcode) {
        case INVALID_CODE: [[fallthrough]];
        case VAL: [[fallthrough]];
        case LBL: [[fallthrough]];
        case DB: [[fallthrough]];
        case DBS:
            return ins;
    }
    
    shaddr += 8;
    
    for (u8 i = 0; i < 3; ++i) {
        if (peek_separator() || peek_eol()) {
            n = next_char();
        }
        if (file.eof() || peek_eol() || n == '\n') {
            return ins;
        }
        
        ins.values[i] = next_value(ins, i);
        
        if (ins.code.ops[i].optype != OP_REG) {
            shaddr += 8;
        }
    }
    return ins;
}

nnasm::op::code asm_compiler::next_opcode() {
    std::string code{};
    while (!peek_separator() && !peek_eol()) {
        code += next_char();
    }
    std::transform(code.begin(), code.end(), code.begin(), ::toupper);
    
    auto it = nnasm::name_to_op.find(code);
    if (it == nnasm::name_to_op.end()) {
        std::stringstream ss{};
        ss << "Invalid opcode \"" << code << "\"";
        error(ss.str());
        return nnasm::op::INVALID_CODE;
    } else {
        return it->second;
    }
}

inline nnasm::op::asm_type asm_type_from_string(std::string str) {
    using namespace std::string_literals;
    
    if (!str.length()) {
        return nnasm::op::ASM_TYPE_QWORD;
    }
    
    switch (str[0]) {
        case '8': {
            if (str.length() == 1) {
                return nnasm::op::ASM_TYPE_BYTE;
            } else if (str == "8S"s) {
                return nnasm::op::ASM_TYPE_SBYTE;
            }
            break;
        }
        case '1': {
            if (str == "16"s) {
                return nnasm::op::ASM_TYPE_WORD;
            } else if (str == "16S"s) {
                return nnasm::op::ASM_TYPE_SWORD;
            }
            break;
        }
        case '3': {
            if (str == "32"s) {
                return nnasm::op::ASM_TYPE_DWORD;
            } else if (str == "32S"s) {
                return nnasm::op::ASM_TYPE_SDWORD;
            }
            break;
        }
        case '6': {
            if (str == "64"s) {
                return nnasm::op::ASM_TYPE_QWORD;
            } else if (str == "64S"s) {
                return nnasm::op::ASM_TYPE_SQWORD;
            }
            break;
        }
        case 'S': {
            if (str.length() == 1) {
                return nnasm::op::ASM_TYPE_QWORD;
            }
            break;
        }
        case 'F': {
            if (str.length() == 1) {
                return nnasm::op::ASM_TYPE_FLOAT;
            }
            break;
        }
        case 'D': {
            if (str.length() == 1) {
                return nnasm::op::ASM_TYPE_DOUBLE;
            }
            break;
        }
    }
    return nnasm::op::ASM_TYPE_INVALID;
}

u64 asm_compiler::value_from_string(const std::string& str, nnasm::instruction_code::operand& op) {
    using namespace std::string_literals;
    std::string val = str;
    vmregister converter{0};
    
    if (u64 uspos = val.find('_'); uspos != std::string::npos) { // Type given
        std::string aft = val.substr(uspos + 1);
        val = val.substr(0, uspos);
        if (aft.length() == 0 || val.length() == 0) {std::stringstream ss{};
            ss << "Invalid value \"" << val << "_" << aft << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
        op.valtype = asm_type_from_string(aft);
    } else { // 64, 64s, or d
        if (val.find('.') != std::string::npos) {
            op.valtype = nnasm::op::ASM_TYPE_DOUBLE;
        } else if (val[0] == '-') {
            op.valtype = nnasm::op::ASM_TYPE_SQWORD;
        } else {
            op.valtype = nnasm::op::ASM_TYPE_QWORD;
        }
    }
    
    if (!(op.valtype & nnasm::op::ASM_TYPE_SIGNED) && val[0] == '-') {
        std::stringstream ss{};
        ss << "Unsigned negative number \"" << val << "\"";
        error(ss.str());
        op.valtype = nnasm::op::ASM_TYPE_INVALID;
        return 0;
    }
    
    if (op.deref && (op.valtype & nnasm::op::ASM_TYPE_FLOATING)) {
        op.deref = nnasm::op::ASM_TYPE_INVALID;
    }
    
    try {
        u64 processed = 0;
        if (op.valtype & nnasm::op::ASM_TYPE_FLOATING) {
            if (op.valtype == nnasm::op::ASM_TYPE_FLOAT) {
                converter.fl = std::stof(val, &processed);
            } else {
                converter.db = std::stod(val, &processed);
            }
            return converter.qword;
        } else {
            int base = 10;
            u64 start = 0;
            if (val[0] == '-') {
                start = 1;
            }
            std::string prefix = val.substr(start, 2);
            if (prefix == "0X"s) {
                base = 16;
                val = val.substr(0, start) + val.substr(start + 2);
            } else if (prefix == "0B"s) {
                base = 2;
                val = val.substr(0, start) + val.substr(start + 2);
            } else if (prefix == "0O"s) {
                base = 8;
                val = val.substr(0, start) + val.substr(start + 2);
            }
            {
                using namespace nnasm::op;
                if (op.valtype & nnasm::op::ASM_TYPE_SIGNED) {
                    converter.sqword = std::stoll(val, &processed, base);
                    switch (op.valtype) {
                        case ASM_TYPE_SBYTE:
                            if (converter.sqword != converter.sbyte) {
                                std::stringstream ss{};
                                ss << "Invalid signed byte \"" << val << "\"";
                                error(ss.str());
                                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                                return 0;
                            }
                            break;
                        case ASM_TYPE_SWORD:
                            if (converter.sqword != converter.sword) {
                                std::stringstream ss{};
                                ss << "Invalid signed word \"" << val << "\"";
                                error(ss.str());
                                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                                return 0;
                            }
                            break;
                        case ASM_TYPE_SDWORD:
                            if (converter.sqword != converter.sdword) {
                                std::stringstream ss{};
                                ss << "Invalid signed dword \"" << val << "\"";
                                error(ss.str());
                                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                                return 0;
                            }
                            break;
                    }
                } else {
                    converter.qword = std::stoull(val, &processed, base);
                    switch (op.valtype) {
                        case ASM_TYPE_BYTE:
                            if (converter.qword != converter.byte) {
                                std::stringstream ss{};
                                ss << "Invalid byte \"" << val << "\"";
                                error(ss.str());
                                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                                return 0;
                            }
                            break;
                        case ASM_TYPE_WORD:
                            if (converter.qword != converter.word) {
                                std::stringstream ss{};
                                ss << "Invalid word \"" << val << "\"";
                                error(ss.str());
                                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                                return 0;
                            }
                            break;
                        case ASM_TYPE_DWORD:
                            if (converter.qword != converter.dword) {
                                std::stringstream ss{};
                                ss << "Invalid dword \"" << val << "\"";
                                error(ss.str());
                                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                                return 0;
                            }
                            break;
                    }
                }
            }
        }
        if (processed != val.length()) {
            std::stringstream ss{};
            ss << "Invalid characters in value \"" << val << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
        return converter.qword;
    } catch (...) {
        std::stringstream ss{};
        ss << "Invalid characters in value \"" << val << "\"";
        error(ss.str());
        op.valtype = nnasm::op::ASM_TYPE_INVALID;
        return 0;
    }
}

u64 asm_compiler::next_value(nnasm::instruction& ins, u8 index, std::string* strptr, bool allow_pending, bool allow_database) {
    using namespace std::string_literals;
    
    vmregister converter{0};
    
    std::string buff{};
    if (!strptr) {
        strptr = &buff;
    }
    
    std::string& val = *strptr;
    while (!peek_separator() && !peek_eol()) {
        val += next_char();
    }
    
    nnasm::instruction_code::operand& op = ins.code.ops[index];
    if (val.length() == 0) {
        op.valtype = nnasm::op::ASM_TYPE_INVALID;
        return 0;
    }
    
    if (val[0] == '<') {
        auto it = literals.find(val.substr(1));
        if (it != literals.end()) {
            val = it->second;
        } else {
            std::stringstream ss{};
            ss << "Cannot find literal \"" << val.substr(1) << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
    }
    
    if (u64 atpos = val.find('@'); atpos != std::string::npos) { // Deref
        std::string bef = val.substr(0, atpos);
        val = val.substr(atpos + 1);
        if (val.length() == 0) {
            std::stringstream ss{};
            ss << "Cannot dereference \"" << val << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
        op.deref = asm_type_from_string(bef);
        if (op.deref == nnasm::op::ASM_TYPE_INVALID) {
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
    }
    
    if (val[0] == '<') {
        auto it = literals.find(val.substr(1));
        if (it != literals.end()) {
            val = it->second;
        } else {
            std::stringstream ss{};
            ss << "Cannot find literal \"" << val.substr(1) << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
    }
    
    if (val[0] == '$') {
        std::transform(val.begin(), val.end(), val.begin(), ::toupper);
        op.optype = nnasm::op::OP_REG;
        if (val.length() < 3 || val.length() > 6) {
            std::stringstream ss{};
            ss << "Invalid register \"" << val << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            op.number = 12;
            return 0;
        }
        std::string reg = val.substr(1, 2);
        val = val.substr(3);
        
        if (reg[0] == 'R') {
            if (reg[1] < 'A' || reg[1] > 'M') {
                std::stringstream ss{};
                ss << "Invalid register \"" << reg << "\"";
                error(ss.str());
                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                op.number = 12;
                return 0;
            }
            op.number = reg[1] - 'A';
        } else if (reg[1] == 'P') {
            if (reg[1] != 'C') {
                std::stringstream ss{};
                ss << "Invalid register \"" << reg << "\"";
                error(ss.str());
                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                op.number = 12;
                return 0;
            } else {
                op.number = 13;
            }
        } else if (reg[2] == 'S') {
            if (reg[1] == 'F') {
                op.number = 14;
            } else if (reg[1] == 'P') {
                op.number = 15;
            } else {
                std::stringstream ss{};
                ss << "Invalid register \"" << reg << "\"";
                error(ss.str());
                op.valtype = nnasm::op::ASM_TYPE_INVALID;
                op.number = 12;
                return 0;
            }
        } else {
            std::stringstream ss{};
            ss << "Invalid register \"" << reg << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            op.number = 12;
            return 0;
        }
        
        op.valtype = asm_type_from_string(val);
        
        return 0;
    } else if (val[0] == '-' || (val[0] >= '0' && val[0] <= '9')){
        std::transform(val.begin(), val.end(), val.begin(), ::toupper);
        op.optype = nnasm::op::OP_VAL;
        return value_from_string(val, op);
    } else if (val[0] == '~') {
        op.optype = nnasm::op::OP_VAL;
        op.valtype = nnasm::op::ASM_TYPE_QWORD;
        val = val.substr(1);
        auto it = database.find(val);
        if (it != database.end()) {
            return it->second.size;
        } else {
            std::stringstream ss{};
            ss << "Cannot find size of \"" << val << "\"";
            error(ss.str());
            op.valtype = nnasm::op::ASM_TYPE_INVALID;
            return 0;
        }
    } else {
        op.optype = nnasm::op::OP_VAL;
        op.valtype = nnasm::op::ASM_TYPE_QWORD;
        {
            auto it = database.find(val);
            if (it != database.end()) {
                if (!allow_database) {
                    std::stringstream ss{};
                    ss << "Illegal reference \"" << val << "\"";
                    error(ss.str());
                    op.valtype = nnasm::op::ASM_TYPE_INVALID;
                }
                if (allow_pending) {
                    pending.push_back({shaddr, "&"s + val});
                }
            } else {
                auto it = labels.find(val);
                if (it != labels.end()) {
                    return it->second;
                } else {
                    if (allow_pending) {
                        pending.push_back({shaddr, val});
                    } else {
                        std::stringstream ss{};
                        ss << "Cannot find \"" << val << "\"";
                        error(ss.str());
                        op.valtype = nnasm::op::ASM_TYPE_INVALID;
                    }
                }
            }
        }
        return 0;
    }
}

char asm_compiler::next_char() {
    char c = file.get();
    char r = c;
    bool skip = false;
    while (true) {
        if (c == EOF) {
            return EOF;
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (c == '\n') {
                ++line;
                r = '\n';
            } else {
                r = ' ';
            }
            char p = file.peek();
            while (p == ' ' || p == '\t' || p == '\n' || p == '\r') {
                if (p == '\n') {
                    ++line;
                    r = '\n';
                }
                c = file.get();
                p = file.peek();
            }
            if (p == EOF) {
                return EOF;
            } else if (skip && r == '\n') {
                return r;
            } else if (!skip && p != ';') {
                return r;
            }
        } else if (c == ';') {
            skip = true;
        } else if (c == '\\') {
            return file.get();
        } else if (!skip) {
            return r;
        }
        c = file.get();
    }
    return r; // Should never be reached anyway
}

bool asm_compiler::peek_separator() {
    char c = file.peek();
    
    return c == ' ' || c == '\r'  || c == '\t';
}

bool asm_compiler::peek_eol() {
    char c = file.peek();
    
    return c == '\n' || c == ';' || c == EOF;
}

void asm_compiler::error(const std::string& msg) {
    using namespace std::string_literals;
    
    std::stringstream ss{};
    ss << filename << ":" << line;
    ss << " - "s << msg;
    
    errors.push_back(ss.str());
}

