#include "frontend/asm_compiler.h"

#include <cmath>
#include <cstring>
#include <regex>
#include "common/utils.h" 
#include "common/type.h"


asm_compiler::asm_compiler(const std::string& filename) : file(filename, std::ios_base::in) {
    if (!file || !file.good()) {
        logger::error() << "Could not open file " << filename << logger::nend;
        // TODO why tho
    }
    
    ready = true;
}

void asm_compiler::compile() {
    using namespace nnasm;
    using namespace nnasm::op;

    bool broken = false;
    u64 addr = 128;
    
    dict<std::vector<u8>> stored_values{};
    dict<std::string> internal_values{};
    dict<u64> labels{};
    std::vector<std::pair<u64, std::string>> unfinished{};
    u64 line{1};
    
    auto until_space = [&]() {
        std::string ret{};
        char c = file.peek();
        while (c != '\t' && c != ' ' && c != '\r' && c != '\n' && !file.eof()) {
            file.get(c);
            ret += c;
            c = file.peek();
        }
        return ret;
    };
    
    auto skip_spaces = [&]() {
        char c = file.peek();
        while (c == '\t' || c == ' ' || c == '\r') {
            file.get(c);
            c = file.peek();
        }
    };
    
    auto skip_to_next = [&]() {
        char c;
        while (!file.eof()) {
            file.get(c);
            if (c == '\n') {
                ++line;
                break;
            }
        }
        skip_spaces();
    };
    
    auto error = [&](const std::string& msg) {
        logger::error() << "Line: " << line << " - " << msg << logger::nend;
        broken = true;
        skip_to_next();
    };
    
    enum class operand_type {
        OPERAND_INVALID, REGISTER_INVALID, SIZE_INVALID, VALUE_INVALID,
        RAW_INVALID,
        
        ADDRESS_INCOMPLETE, VALUE_INCOMPLETE, 
        
        UNSIGNED_REGISTER, SIGNED_REGISTER, FLOAT_REGISTER, 
        ADDRESS, SIZE, UNSIGNED_INTEGER, SIGNED_INTEGER, REAL
    };
    
    auto number_to_raw = [](auto n) {
        std::vector<u8> ret{};
        u8* dat = reinterpret_cast<u8*>(&n);
        for (u8 i = 0; i < sizeof(n); ++i) {
            ret.push_back(dat[i]);
        }
        return ret;
    };
    
    std::function<operand_type(const std::string&)> check_format;
    check_format = [&](const std::string& data) {
        if (!data.length()) {
            return operand_type::OPERAND_INVALID;
        }
        std::regex integer{"^(0x[0-9a-fA-F]+|0b[01]+|0o[0-7]+|-?[0-9]+)(_(8|16|32|64|f|d)s?|s)?$"};
        std::regex real{"^-?[0-9]+\\.[0-9]+(_(d|f))?$"};
        switch (data[0]) {
            case '$': { // Register
                std::regex reg{"^\\$(r[a-m]|pc|sp|sf)(64|s|8s?|16s?|32s?|f|d)?$"};
                if (!std::regex_match(data, reg)) {
                    return operand_type::REGISTER_INVALID;
                } else if (data.length() > 3) {
                    if (data.back() == 's') {
                        return operand_type::SIGNED_REGISTER;
                    } else if (data.back() == 'f' || data.back() == 'd') {
                        return operand_type::FLOAT_REGISTER;
                    } else {
                        return operand_type::UNSIGNED_REGISTER;
                    }
                } else {
                    return operand_type::UNSIGNED_REGISTER;
                }
            }
            case '@': { // Address
                std::string raddr = data.substr(1);
                if (std::regex_match(data, integer)) {
                    return operand_type::ADDRESS;
                }
                if (labels.find(raddr) == labels.end()) {
                    return operand_type::ADDRESS_INCOMPLETE;
                } else {
                    return operand_type::ADDRESS;
                }
            }
            case '~': { // Size of data
                std::string rdata = data.substr(1);
                auto value = stored_values.find(rdata);
                if (value == stored_values.end()) {
                    return operand_type::SIZE_INVALID;
                }
                return operand_type::SIZE;
            }
            case '<': { // internal value
                std::string rdata = data.substr(1);
                auto value = internal_values.find(rdata);
                if (value == internal_values.end()) {
                    return operand_type::VALUE_INVALID;
                }
                return check_format(value->second);
            }
            case '1': [[fallthrough]];
            case '2': [[fallthrough]];
            case '3': [[fallthrough]];
            case '4': [[fallthrough]];
            case '5': [[fallthrough]];
            case '6': [[fallthrough]]; 
            case '7': [[fallthrough]];
            case '8': [[fallthrough]];
            case '9': [[fallthrough]];
            case '0': {
                if (std::regex_match(data, real)) {
                    return operand_type::REAL;
                } else if (std::regex_match(data, integer)) {
                    bool sign = data.find_first_of("-s") != std::string::npos;
                    return sign ? operand_type::SIGNED_INTEGER : operand_type::UNSIGNED_INTEGER;
                } else {
                    return operand_type::RAW_INVALID;
                }
            }
            default: {
                if (labels.find(data) == labels.end()) {
                    return operand_type::VALUE_INCOMPLETE;
                } else {
                    return operand_type::UNSIGNED_INTEGER;
                }
            }
        }
    };
    
    std::function<std::vector<u8>(const std::string&, bool)> to_raw;
    to_raw = [&](const std::string& data, bool force_64) -> std::vector<u8> {
        using namespace std::string_literals;
        if (!data.length()) {
            return {};
        }
        switch (data[0]) {
            case '$': { // Register
                std::string rname = data.substr(1, 2);
                u8 val = 0;
                if (rname == "sp"s) {
                    val = 15;
                } else if (rname == "sf"s) {
                    val = 14;
                } else if (rname == "pc"s) {
                    val = 13;
                } else {
                    val = data[2] - 'a';
                }
                bool sign = data.back() == 's';
                std::string part = data;
                if (sign) {
                    part = data.substr(0, data.length() - 1);
                    val |= REG_SIGNED;
                } 
                part = part.substr(3);
                if (part == "8"s) {
                    val |= REG_BYTE;
                } else if (part == "16"s) {
                    val |= REG_WORD;
                } else if (part == "32"s) {
                    val |= REG_DWORD;
                } else if (part == "f"s) {
                    val |= REG_FLOAT;
                } else if (part == "d"s) {
                    val |= REG_DOUBLE;
                } else {
                    val |= REG_QWORD;
                }
                return {val};
            }
            case '@': { // Address
                if (data[1] >= '0' && data[1] <= '9') {
                    u64 addr = std::stoull(data.substr(1));
                    return number_to_raw(addr);
                } else {
                    auto lbl = labels.find(data.substr(1));
                    if (lbl == labels.end()) {
                        return number_to_raw((u64) 0ull);
                    } else {
                        return number_to_raw(lbl->second);
                    }
                }
            }
            case '~': { // Size of data
                auto value = stored_values.find(data.substr(1));
                return number_to_raw((u64) value->second.size());
            }
            case '<': { // internal value
                auto value = internal_values.find(data.substr(1));
                return to_raw(value->second, force_64);
            }
            case '1': [[fallthrough]];
            case '2': [[fallthrough]];
            case '3': [[fallthrough]];
            case '4': [[fallthrough]];
            case '5': [[fallthrough]];
            case '6': [[fallthrough]]; 
            case '7': [[fallthrough]];
            case '8': [[fallthrough]];
            case '9': [[fallthrough]];
            case '0': {
                u8 type = 3;
                std::string number = data;
                bool sign = false;
                if (data.back() == 's') {
                    sign = true;
                    number = data.substr(0, data.length() - 1);
                }
                if (u64 pos = number.find('_'); pos != std::string::npos) {
                    std::string suffix = number.substr(pos);
                    number = number.substr(0, pos);
                    if (!force_64) {
                        if (suffix == "_8"s) {
                            type = 0;
                        } else if (suffix == "_16"s) {
                            type = 1;
                        } else if (suffix == "_32"s) {
                            type = 2;
                        } else if (suffix == "_64"s) {
                            type = 3;
                        } else if (suffix == "_f"s) {
                            type = 4;
                        } else if (suffix == "_d"s) {
                            type = 5;
                        }
                    }
                }
                if (type == 4) {
                    return number_to_raw(std::stof(number));
                } else if (type == 5) {
                    return number_to_raw(std::stod(number));
                } else if (sign) {
                    i64 bign;
                    if (number.find("0x") == 0) {
                        bign = std::stoll(number.substr(2), nullptr, 16);
                    } else if (number.find("0o") == 0) {
                        bign = std::stoll(number.substr(2), nullptr, 8);
                    } else if (number.find("0b") == 0) {
                        bign = std::stoll(number.substr(2), nullptr, 2);
                    } else {
                        bign = std::stoll(number);
                    }
                    if (type == 0) {
                        return number_to_raw((i8) bign);
                    } else if (type == 1) {
                        return number_to_raw((i16) bign);
                    } else if (type == 2) {
                        return number_to_raw((i32) bign);
                    } else {
                        return number_to_raw(bign);
                    }
                } else {
                    u64 bign;
                    if (number.find("0x") == 0) {
                        bign = std::stoull(number.substr(2), nullptr, 16);
                    } else if (number.find("0o") == 0) {
                        bign = std::stoull(number.substr(2), nullptr, 8);
                    } else if (number.find("0b") == 0) {
                        bign = std::stoull(number.substr(2), nullptr, 2);
                    } else {
                        bign = std::stoull(number);
                    }
                    if (type == 0) {
                        return number_to_raw((u8) bign);
                    } else if (type == 1) {
                        return number_to_raw((u16) bign);
                    } else if (type == 2) {
                        return number_to_raw((u32) bign);
                    } else {
                        return number_to_raw(bign);
                    }
                }
            }
            default: {
                auto lbl = labels.find(data);
                if (lbl == labels.end()) {
                    return number_to_raw((u64) 0ull);
                } else {
                    return number_to_raw(lbl->second);
                }
            }
        }
    };
    
    skip_spaces();
    char c;
    
    compiled.resize(addr);
    
    while (!file.eof()) {
        c = file.peek();
        switch (c) {
            case ';':
                skip_to_next();
                continue;
            case ' ': [[fallthrough]];
            case '\t':
                skip_spaces();
                continue;
            case '\n': [[fallthrough]];
            case '\r':
                skip_to_next();
                continue;
            case EOF:
                goto out_while;
            default:
                break;
        }
        
        // We are at an instruction
        std::string opcodestr = until_space();
        auto elem = nnasm::name_to_op.find(opcodestr);
        code opcode = elem != nnasm::name_to_op.end() ? elem->second : INVALID_CODE;
        
        switch (opcode) {
            case VAL: {
                skip_spaces();
                std::string name = until_space();
                skip_spaces();
                std::string val = until_space();
                if (name.empty() || (name[0] >= '0' && name[0] <= '9')) {
                    error("Invalid name given");
                    continue;
                } else if (val.empty()) {
                    error("No value given");
                }
                auto valtype = check_format(val);
                if (valtype >= operand_type::OPERAND_INVALID && valtype <= operand_type::RAW_INVALID) {
                    error("Invalid value given");
                }
                internal_values.insert({name, val});
                continue;
            }
            case DB: {
                skip_spaces();
                std::string name = until_space();
                if (name.empty() || (name[0] >= '0' && name[0] <= '9')) {
                    error("Invalid value name given");
                    continue;
                }
                skip_spaces();
                std::vector<u8> data{};
                bool fine = true;
                do {
                    std::string value = until_space();
                    auto valtype = check_format(value);
                    if (valtype < operand_type::UNSIGNED_INTEGER && valtype > operand_type::REAL) {
                        error("Invalid value given");
                        fine = false;
                        break;
                    } else {
                        auto nd = to_raw(value, false);
                        data.insert(data.end(), nd.begin(), nd.end());
                    }
                    skip_spaces();
                    if (file.peek() == ';' || file.peek() == '\n' || file.eof()) {
                        break;
                    }
                } while (true);
                if (fine) {
                    stored_values.insert({name, data});
                }
                continue;
            }
            case DBS: {
                skip_spaces();
                std::string name = until_space();
                if (name.empty() || (name[0] >= '0' && name[0] <= '9')) {
                    error("Invalid value name given");
                    continue;
                }
                skip_spaces();
                std::vector<u8> data{}; // TODO this is all wrong
                bool fine = true;
                do {
                    std::string value = until_space();
                    if (value[0] == '"') {
                        u64 i = 1;
                        while (true) {
                            if (i >= value.size()) {
                                break;
                            }
                            char cc = value[i];
                            if(cc == '\\') {
                                ++i;
                                if (i >= value.size()) {
                                    break;
                                }
                                cc = value[i];
                                switch (cc) {
                                    case 'n':
                                        data.push_back('\n');
                                        break;
                                    case 't':
                                        data.push_back('\t');
                                        break;
                                    case 'r':
                                        data.push_back('\r');
                                        break;
                                    case '0':
                                        data.push_back('\0');
                                        break;
                                    default:
                                        data.push_back(cc);
                                        break;
                                }
                            } else if (cc == '"') {
                                break;
                            } else {
                                data.push_back(cc);
                            }
                            ++i;
                        }
                        if (i != value.size() || value[value.size() - 1] != '"') {
                            error("Invalid value given");
                            fine = false;
                            break;
                        }
                    } else {
                        auto valtype = check_format(value);
                        if (valtype < operand_type::UNSIGNED_INTEGER && valtype > operand_type::REAL) {
                            error("Invalid value given");
                            fine = false;
                            break;
                        } else {
                            auto nd = to_raw(value, false);
                            data.insert(data.end(), nd.begin(), nd.end());
                        }
                    }
                    skip_spaces();
                    if (file.peek() == ';' || file.peek() == '\n' || file.eof()) {
                        break;
                    }
                } while (true);
                if (fine) {
                    auto strsize = number_to_raw((u64) data.size());
                    auto strtype = number_to_raw((u64) etype_ids::STRING);
                    data.insert(data.begin(), strsize.begin(), strsize.end());
                    data.insert(data.begin(), strtype.begin(), strtype.end());
                    stored_values.insert({name, data});
                }
                continue;
            }
            case LBL: {
                skip_spaces();
                std::string label = until_space();
                if (label.empty() || (label[0] >= '0' && label[0] <= '9')) {
                    error("Invalid label name given");
                    continue;
                }
                labels.insert({label, addr});
                continue;
            }
            case INVALID_CODE:
                error("Uknknown opcode"); // TODO Better
                continue;
            default:
                break;
        }
        
        instruction ins;
        ins.code = opcode;
        
        addr += 2;
        auto formats = nnasm::format::instructions.at(opcode);
        skip_spaces();
        
        using opformat = nnasm::format::operand_format;
        
        std::vector<u8> opdata{};
        std::string op[3];
        opformat optyp[3] {opformat::OP_NONE, opformat::OP_NONE, opformat::OP_NONE};
        for (int i = 0; i < 3; ++i) {
            if (file.peek() == ';' || file.peek() == '\n' || file.eof()) {
                break;
            }
            op[i] = until_space();
            skip_spaces();
            
            auto optype = check_format(op[i]);
            switch (optype) {
                case operand_type::OPERAND_INVALID: [[fallthrough]];
                case operand_type::RAW_INVALID: [[fallthrough]];
                case operand_type::SIZE_INVALID: [[fallthrough]];
                case operand_type::VALUE_INVALID: 
                    error("Invalid value");
                    optyp[i] = opformat::OP_NONE;
                    break;
                case operand_type::REGISTER_INVALID:
                    error("Invalid register");
                    optyp[i] = opformat::OP_NONE;
                    break;
                case operand_type::ADDRESS_INCOMPLETE:
                    optyp[i] = opformat::OP_ADDR;
                    unfinished.push_back({addr, op[i].substr(1)});
                    addr += 8;
                    break;
                case operand_type::VALUE_INCOMPLETE:
                    optyp[i] = opformat::OP_VAL | opformat::OP_FLAGS_UINT;
                    unfinished.push_back({addr, op[i]});
                    addr += 8;
                    break;
                case operand_type::ADDRESS:
                    optyp[i] = opformat::OP_ADDR;
                    addr += 8;
                    break;
                case operand_type::REAL:
                    optyp[i] = opformat::OP_VAL | opformat::OP_FLAGS_FLOAT;
                    addr += 8;
                    break;
                case operand_type::SIGNED_INTEGER: 
                    optyp[i] = opformat::OP_VAL | opformat::OP_FLAGS_SINT;
                    addr += 8;
                    break;
                case operand_type::UNSIGNED_INTEGER: [[fallthrough]];
                case operand_type::SIZE:
                    optyp[i] = opformat::OP_VAL | opformat::OP_FLAGS_UINT;
                    addr += 8;
                    break;
                case operand_type::SIGNED_REGISTER:
                    optyp[i] = opformat::OP_REG | opformat::OP_FLAGS_SINT;
                    ++addr;
                    break;                    
                case operand_type::UNSIGNED_REGISTER:
                    optyp[i] = opformat::OP_REG | opformat::OP_FLAGS_UINT;
                    ++addr;
                    break;
                case operand_type::FLOAT_REGISTER:
                    optyp[i] = opformat::OP_REG | opformat::OP_FLAGS_FLOAT;
                    ++addr;
                    break;
            }
            
            if (optyp[i] != opformat::OP_NONE) {
                auto val = to_raw(op[i], true);
                opdata.insert(opdata.end(), val.begin(), val.end());
            }
        }
        
        ins.op1 = (optyp[0] & opformat::OP_TYPE) == opformat::OP_ADDR ? OP_ADDR : (optyp[0] & opformat::OP_TYPE);
        ins.op2 = (optyp[1] & opformat::OP_TYPE) == opformat::OP_ADDR ? OP_ADDR : (optyp[1] & opformat::OP_TYPE);
        ins.op3 = (optyp[2] & opformat::OP_TYPE) == opformat::OP_ADDR ? OP_ADDR : (optyp[2] & opformat::OP_TYPE);
        
        bool found = false;
        
        constexpr auto complies = [](opformat inst, opformat form) {
            if (form == opformat::OP_NONE) {
                return inst == opformat::OP_NONE;
            }
            u8 instype = inst & opformat::OP_TYPE;
            u8 insflags = inst & opformat::OP_FLAGS;
            u8 type_complies = instype & form;
            u8 flag_complies = insflags & form;
            if (!type_complies) {
                return false;
            }
            if ((form & opformat::OP_FLAGS) && !flag_complies) {
                return false;
            }
            return true;
        };
        
        for (auto& format : formats) {
            if (complies(optyp[0], format.op1) && complies(optyp[1], format.op2) && complies(optyp[2], format.op3)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            error("Instruction with those operands does not exist");
        } 
        auto insdata = number_to_raw(ins.raw);
        opdata.insert(opdata.begin(), insdata.begin(), insdata.end());
        compiled.insert(compiled.end(), opdata.begin(), opdata.end());
    }
    out_while:;
    
    u64 pad = 8 * std::ceil((double) addr / 8.0);
    if (pad > addr) {
        compiled.resize(pad);
        std::fill(compiled.begin() + addr, compiled.end(), 0x00);
        addr = pad;
    }
    
    for (auto& [name, data] : stored_values) {
        auto lbl = labels.find(name);
        if (lbl != labels.end()) {
            error("Label exists with same name as stored value");
        } else {
            u64 prev_size = data.size();
            u64 size = 8 * std::ceil((double) prev_size / 8.0);
            labels.insert({name, addr});
            if (prev_size < size) {
                data.resize(size);
                std::fill(data.begin() + prev_size, data.end(), 0x00);
            }
            compiled.insert(compiled.end(), data.begin(), data.end());
            addr += size;
        }
    }
    
    for (auto& [addr, name] : unfinished) {
        auto lbl = labels.find(name);
        if (lbl == labels.end()) {
            error("Label does not exist");
        } else {
            auto val = number_to_raw(lbl->second);
            for (int i = 0; i < val.size(); ++i) {
                compiled[addr + i] = val[i];
            }
        }
    }
    
    struct { // TODO Clean up, my god
        char magic[4] = {'N', 'N', 'E', 'P'};
        u32 version = 0;
        u64 start = 128;
        u64 size = 0;
        u64 initial = 4 << 20;
    } header;
    header.size = compiled.size();
    auto headerdata = number_to_raw(header);
    headerdata.resize(128);
    std::fill(headerdata.begin() + sizeof(header), headerdata.end(), 0x00);
    for (int i = 0; i < headerdata.size(); ++i) {
        compiled[i] = headerdata[i];
    }
    
}

u8* asm_compiler::get() {
    return compiled.data();
}

u8* asm_compiler::move() {
    if (!compiled.size()) {
        return nullptr;
    }
    
    u8* data = new u8[compiled.size()];
    std::memcpy(data, compiled.data(), sizeof(u8) * compiled.size());
    compiled.clear();
    return data;
}

void asm_compiler::store_to_file(const std::string& filename) {
    std::ofstream file{filename, std::ios_base::out | std::ios_base::binary};
    
    if (!file || !file.good()) {
        logger::error() << "Could not open file " << filename << logger::nend;
        return;
    }
}
