#include "vm/machine.h"

#include <cstdlib>
#include <type_traits>
#include "frontend/parser.h"
#include "common/utils.h"

#define instruction_lambda(NAME, ...) \
static const auto NAME = [this](auto& a, auto& b, auto& c) {\
    using atype = std::remove_reference_t<decltype(a)>;\
    using btype = std::remove_reference_t<decltype(b)>;\
    using ctype = std::remove_reference_t<decltype(c)>;\
    { __VA_ARGS__ }\
}

using vm = virtualmachine;

vm::virtualmachine(parser& p) : p(p) {
    
}

vm::~virtualmachine() {
    clear();
}

void vm::run() {
    stopped = false;
    start_time = std::chrono::high_resolution_clock::now();
    while (running && !stopped) {
        step();
    }
    if (!running) {
        end_time = std::chrono::high_resolution_clock::now();
    }
}

void vm::step() {
//     auto execute_start = std::chrono::high_resolution_clock::now();
    execute_instruction();
    ++steps;
//     auto load_start = std::chrono::high_resolution_clock::now();
    load_instruction();
//     auto end_time = std::chrono::high_resolution_clock::now();
    
//     load_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - load_start).count();
//     execute_time += std::chrono::duration_cast<std::chrono::microseconds>(load_start - execute_start).count();
}

void vm::load_instruction() {
    u64 loc64 = pc.qword >> 3; // Divide by 8, pc must always be aligned
    u8 ops{1};
    loaded.code.raw = memory64[loc64];
    for (int i = 0; i < 3; ++i) {
        if (loaded.code.ops[i].optype == nnasm::op::OP_NONE) {
            break;
        } else if (loaded.code.ops[i].optype == nnasm::op::OP_REG) {
            continue;
        } else {
            loaded.values[i] = memory64[loc64 + ops];
            ++ops;
        }
    }
    pc.qword += ops << 3;
//     std::memcpy(&loaded, memory + pc.qword, sizeof(loaded));
//     pc.qword += 32;
}

void vm::execute_instruction() {
    using namespace nnasm::op;
    auto& op0 = loaded.code.ops[0];
    auto& op1 = loaded.code.ops[1];
    auto& op2 = loaded.code.ops[2];
    
    switch (loaded.code.opcode) {
        case NOP:
            break;
        case MOV: {
            if (op1.optype == OP_REG) {
                instruction_lambda(mov, 
                    b = static_cast<btype>(a);
                );
                ins2op(op2, mov);
                break;
            } else {
                instruction_lambda(mov, 
                    u64 addr = absolute_address(b);
                    if (addr < read_only_end || addr > allocated) {
                        trap(0);
                        return;
                    }
                    *reinterpret_cast<atype*>(memory + addr) = a;
                );
                ins2op(op2, mov);
                break;
            }
        }
        case MVI: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(mvi, 
                    u64 addr = absolute_address(-sp.qword + a);
                    if (addr > allocated) {
                        trap(0);
                        return;
                    }
                    b = *reinterpret_cast<btype*>(memory + addr);
                );
                ins2op(op2, mvi);
                break;
            } else {
                instruction_lambda(mvi, 
                    u64 addr = absolute_address(a + b);
                    if (addr > allocated) {
                        trap(0);
                        return;
                    }
                    c = *reinterpret_cast<ctype*>(memory + addr);
                );
                ins3op(mvi);
                break;
            }
            break;
        }
        case CPY: {
            instruction_lambda(cpy, 
                u64 from = absolute_address(a);
                u64 to   = absolute_address(b);
                if (from < read_only_end || to < read_only_end || from + c > allocated || to + c > allocated) {
                    trap(0);
                    return;
                }
                if (from + c > to) {
                    std::memmove(memory + to, memory + from, c);
                } else {
                    std::memcpy(memory + to, memory + from, c);
                }
            );
            ins3op_int(cpy);
            break;
        }
        case ZRO: {
            instruction_lambda(zro, 
                u64 from = absolute_address(a);
                if (from < read_only_end || from + b > allocated) {
                    trap(0);
                    return;
                }
                std::memset(memory + from, 0, b);
            );
            ins2op_int(op2, zro);
            break;
        }
        case SET: {
            instruction_lambda(set, 
                u64 from = absolute_address(b);
                if (from < read_only_end || from + b > allocated) {
                    trap(0);
                    return;
                }
                std::memset(memory + from, a, c);
            );
            ins3op_int(set);
            break;
        }
        case BRK: {
            trap(-1);
            stopped = true;
            break;
        }
        case HLT: {
            trap(-2);
            stopped = true;
            running = false;
            break;
        }
        case CZRO: {
            instruction_lambda(czro, 
                sf.check = a == 0;
            );
            ins1op(op1, op2, czro);
            break;
        }
        case CNZR: {
            instruction_lambda(cnzr, 
                sf.check = a != 0;
            );
            ins1op(op1, op2, cnzr);
            break;
        }
        case CEQ: {
            instruction_lambda(ceq, 
                sf.check = a == b;
            );
            ins2op(op2, ceq);
            break;
        }
        case CNEQ: {
            instruction_lambda(cneq, 
                sf.check = a != b;
            );
            ins2op(op2, cneq);
            break;
        }
        case CLT: {
            instruction_lambda(clt, 
                sf.check = a < b;
            );
            ins2op(op2, clt);
            break;
        }
        case CLE: {
            instruction_lambda(cle, 
                sf.check = a <= b;
            );
            ins2op(op2, cle);
            break;
        }
        case CGT: {
            instruction_lambda(cgt, 
                sf.check = a > b;
            );
            ins2op(op2, cgt);
            break;
        }
        case CGE: {
            instruction_lambda(cge, 
                sf.check = a >= b;
            );
            ins2op(op2, cge);
            break;
        }
        case CBS: {
            instruction_lambda(cbs, 
                sf.check = (a & (1 << b));
            );
            ins2op_int(op2, cbs);
            break;
        }
        case CBNS: {
            instruction_lambda(cbns, 
                sf.check = !(a & (1 << b));
            );
            ins2op_int(op2, cbns);
            break;
        }
        case JMP: {
            instruction_lambda(jmp,
                u64 addr = absolute_address(a);
                if (addr > allocated) {
                    trap(0);
                    return;
                }
                pc.qword = addr;
            );
            ins1op_int(op1, op2, jmp);
            break;
        }
        case JMPR: {
            instruction_lambda(jmpr,
                if (pc.qword + a > allocated) {
                    trap(0);
                    return;
                }
                pc.qword += a;
            );
            ins1op_int(op1, op2, jmpr);
            break;
        }
        case JCH: {
            instruction_lambda(jch,
                if (!sf.check) {
                    return;
                }
                u64 addr = absolute_address(a);
                if (addr > allocated) {
                    trap(0);
                    return;
                }
                pc.qword = addr;
            );
            ins1op_int(op1, op2, jch);
            break;
        }
        case JNCH: {
            instruction_lambda(jnch,
                if (sf.check) {
                    return;
                }
                u64 addr = absolute_address(a);
                if (addr > allocated) {
                    trap(0);
                    return;
                }
                pc.qword = addr;
            );
            ins1op_int(op1, op2, jnch);
            break;
        }
        case PUSH: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(ipush,
                    push(a);
                );
                ins1op_int(op1, op2, ipush);
                break;
            } else {
                instruction_lambda(ipush,
                    u64 addr = absolute_address(a);
                    if (addr > allocated || addr < read_only_end || addr + b > (allocated - stack_size + b)) {
                        trap(0);
                        return;
                    }
                    if (sp.qword + b > stack_size) {
                        trap(2);
                        return;
                    }
                    sp.qword += b;
                    std::memcpy(end - sp.qword, memory + addr, b);
                );
                ins2op_int(op2, ipush);
                break;
            }
        }
        case POP: {
            if (op1.optype != OP_NONE) {
                instruction_lambda(ipop,
                    u64 addr = absolute_address(a);
                    if (addr > allocated || addr < read_only_end || addr + b > (allocated - stack_size + b)) {
                        trap(0);
                        return;
                    }
                    if (sp.qword - b > stack_size) {
                        trap(3);
                        return;
                    }
                    std::memcpy(memory + addr, end - sp.qword, b);
                    sp.qword -= b;
                );
                ins2op_int(op2, ipop);
                break;
            } else if (op1.optype == OP_REG) {
                instruction_lambda(ipop,
                    a = pop<atype>();
                );
                ins1op_int(op1, op2, ipop);
                break;
            } else {
                instruction_lambda(ipop,
                    if (sp.qword - a > stack_size) {
                        trap(3);
                        return;
                    }
                    sp.qword -= a;
                );
                ins1op_int(op1, op2, ipop);
                break;
            }
        }
        case BTIN: {
            instruction_lambda(btin, 
                // Call a somehow
            );
            ins1op_int(op1, op2, btin);
            break;
        }
        case CALL: {
            instruction_lambda(call, 
                push(pc.qword);
                pc.qword = a;
            );
            ins1op_int(op1, op2, call);
            break;
        }
        case RET: {
            instruction_lambda(ret, 
                pc.qword = pop<u64>();
            );
            ins0op(op0, op1, op2, ret);
            break;
        }
        case DRF: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(drf, 
                    if constexpr (std::is_floating_point_v<atype>) {
                        trap(1);
                    } else {
                        u64 addr = absolute_address(a);
                        if (addr > allocated) {
                            trap(0);
                            return;
                        }
                        a = *reinterpret_cast<atype*>(memory + addr);
                    }
                );
                ins1op(op1, op2, drf);
                break;
            } else {
                instruction_lambda(drf, 
                    if constexpr (std::is_floating_point_v<atype>) {
                        trap(1);
                    } else {
                        u64 addr = absolute_address(a);
                        if (addr > allocated) {
                            trap(0);
                            return;
                        }
                        b = *reinterpret_cast<atype*>(memory + addr);
                    }
                );
                ins2op(op2, drf);
                break;
            }
        }
        case IDX: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(idx, 
                    if constexpr (std::is_floating_point_v<atype> || std::is_floating_point_v<btype>) {
                        trap(1);
                    } else {
                        u64 addr = absolute_address(b);
                        if (addr > allocated) {
                            trap(0);
                            return;
                        }
                        u64 faddr = addr + (a * sizeof(btype));
                        b = *reinterpret_cast<atype*>(memory + addr);
                    }
                );
                ins2op(op2, idx);
            } else {
                instruction_lambda(idx, 
                    if constexpr (std::is_floating_point_v<atype> || std::is_floating_point_v<btype>) {
                        trap(1);
                    } else {
                        u64 addr = absolute_address(b);
                        if (addr > allocated) {
                            trap(0);
                            return;
                        }
                        u64 faddr = addr + (a * sizeof(btype));
                        c = *reinterpret_cast<atype*>(memory + addr);
                    }
                );
                ins3op(idx);
            }
            break;
        }
        case ADD: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(add, 
                    b += static_cast<btype>(a);
                );
                ins2op(op2, add);
            } else {
                instruction_lambda(add, 
                    c = static_cast<ctype>(b + static_cast<btype>(a));
                );
                ins3op(add);
            }
            break;
        }
        case INC: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(inc, 
                    ++a;
                );
                ins1op_int(op1, op2, inc);
            } else {
                instruction_lambda(inc, 
                    b = a++;
                );
                ins2op_int(op2, inc);
            }
            break;
        }
        case SUB: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(sub, 
                    b -= static_cast<btype>(a);
                );
                ins2op(op2, sub);
            } else {
                instruction_lambda(sub, 
                    c = static_cast<ctype>(b - static_cast<btype>(a));
                );
                ins3op(sub);
            }
            break;
        }
        case DEC: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(dec, 
                    --a;
                );
                ins1op_int(op1, op2, dec);
            } else {
                instruction_lambda(dec, 
                    b = a--;
                );
                ins2op_int(op2, dec);
            }
            break;
        }
        case MUL: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(mul, 
                    b *= static_cast<btype>(a);
                );
                ins2op(op2, mul);
            } else {
                instruction_lambda(mul, 
                    c = static_cast<ctype>(b * static_cast<btype>(a));
                );
                ins3op(mul);
            }
            break;
        }
        case DIV: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(div, 
                    b /= static_cast<btype>(a);
                );
                ins2op(op2, div);
            } else {
                instruction_lambda(div, 
                    c = static_cast<ctype>(b / static_cast<btype>(a));
                );
                ins3op(div);
            }
            break;
        }
        case MOD: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(mod, 
                    b %= static_cast<btype>(a);
                );
                ins2op_int(op2, mod);
            } else {
                instruction_lambda(mod, 
                    c = static_cast<ctype>(b % static_cast<btype>(a));
                );
                ins3op_int(mod);
            }
            break;
        }
        case ABS: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(absi, 
                    a = a > 0 ? a : -a;
                );
                ins1op(op1, op2, absi);
            } else {
                instruction_lambda(absi, 
                    b = a > 0 ? a : -a;
                );
                ins2op(op2, absi);
            }
            break;
        }
        case NEG: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(neg, 
                    a = -a;
                );
                ins1op(op1, op2, neg);
            } else {
                instruction_lambda(neg, 
                    b = -a;
                );
                ins2op(op2, neg);
            }
            break;
        }
        case SHR: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(shr, 
                    b >>= a;
                );
                ins2op_int(op2, shr);
            } else {
                instruction_lambda(shr, 
                    c = b >> a;
                );
                ins3op_int(shr);
            }
            break;
        }
        case SHL: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(shl, 
                    b <<= a;
                );
                ins2op_int(op2, shl);
            } else {
                instruction_lambda(shl, 
                    c = b << a;
                );
                ins3op_int(shl);
            }
            break;
        }
        case RTR: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(rtr, 
                    if (a > sizeof(btype)) {
                        a %= sizeof(btype);
                    }
                    b = b >> a | b << (sizeof(btype) - a);
                );
                ins2op_int(op2, rtr);
            } else {
                instruction_lambda(rtr, 
                    if (a > sizeof(btype)) {
                        a %= sizeof(btype);
                    }
                    c = b >> a | b << (sizeof(btype) - a);
                );
                ins3op_int(rtr);
            }
            break;
        }
        case RTL: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(rtl, 
                    if (a > sizeof(btype)) {
                        a %= sizeof(btype);
                    }
                    b = b << a | b >> (sizeof(btype) - a);
                );
                ins2op_int(op2, rtl);
            } else {
                instruction_lambda(rtl, 
                    if (a > sizeof(btype)) {
                        a %= sizeof(btype);
                    }
                    c = b << a | b >> (sizeof(btype) - a);
                );
                ins3op_int(rtl);
            }
            break;
        }
        case AND: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(andi, 
                    b &= a;
                );
                ins2op_int(op2, andi);
            } else {
                instruction_lambda(andi, 
                    c = b & a;
                );
                ins3op_int(andi);
            }
            break;
        }
        case OR: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(ori, 
                    b |= a;
                );
                ins2op_int(op2, ori);
            } else {
                instruction_lambda(ori, 
                    c = b | a;
                );
                ins3op_int(ori);
            }
            break;
        }
        case XOR: {
            if (op2.optype == OP_NONE) {
                instruction_lambda(andi, 
                    b ^= a;
                );
                ins2op_int(op2, andi);
            } else {
                instruction_lambda(andi, 
                    c = b ^ a;
                );
                ins3op_int(andi);
            }
            break;
        }
        case NOT: {
            if (op1.optype == OP_NONE) {
                instruction_lambda(noti, 
                    a = ~a;
                );
                ins1op_int(op1, op2, noti);
            } else {
                instruction_lambda(noti, 
                    b = ~a;
                );
                ins2op_int(op2, noti);
            }
            break;
        }
        default: {
            trap(4);
            break;
        }
    }
}

void vm::stop() {
    stopped = true;
}

void vm::load(u8* program, u64 size) {
    clear();
    u64* hdr = reinterpret_cast<u64*>(program); // TODO Proper header struct please
    u64 code_start = hdr[1];
    u64 data_start = hdr[2];
    u64 file_size  = hdr[3];
    u64 init_alloc = hdr[4]; 
    
    allocate(file_size + init_alloc);
    
    memory64 = reinterpret_cast<u64*>(memory);
    code = memory + code_start;
    read_only_end = data_start;
    data = memory + data_start;
    heap = memory + file_size;
    stack_size = std::min(init_alloc >> 1, (u64) 4 << 16); // 4 MB
    
    std::memcpy(memory, program, file_size);
    pc.qword = code_start;
    running = true;
    load_instruction();
}

void vm::allocate(u64 amount) {
    if (memory) {
        u64 code_diff = code - memory;
        u64 data_diff = data - memory;
        u64 heap_diff = heap - memory;
        u8* buff = (u8*) std::aligned_alloc(sizeof(u64), allocated + amount + 8);
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy(buff + allocated + amount - stack_size, end - stack_size, stack_size);
        std::free(memory);
        memory = buff;
        allocated += amount;
        memory64 = reinterpret_cast<u64*>(memory);
        code = memory + code_diff;
        data = memory + data_diff;
        heap = memory + heap_diff;
        stack_size = std::min(allocated >> 1, (u64) 4 << 16); // 4 MB
        end  = memory + allocated + 1;
    } else {
        memory = (u8*) std::aligned_alloc(sizeof(u64), amount + 8); // TODO assume non-infinite amount of memory
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void vm::resize(u64 amount) {
    if (memory) {
        u64 code_diff = code - memory;
        u64 data_diff = data - memory;
        u64 heap_diff = heap - memory;
        u8* buff = (u8*) std::aligned_alloc(sizeof(u64), amount + 8);
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy(buff + amount - stack_size, end - stack_size, stack_size);
        std::free(memory);
        memory = buff;
        allocated = amount;
        memory64 = reinterpret_cast<u64*>(memory);
        code = memory + code_diff;
        data = memory + data_diff;
        heap = memory + heap_diff;
        stack_size = std::min(allocated >> 1, (u64) 4 << 16); // 4 MB TODO Fix stacks and heaps growth
        end  = memory + allocated + 1;
    } else {
        memory = (u8*) std::aligned_alloc(sizeof(u64), amount + 8);
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void vm::clear() {
    if (memory) {
        std::free(memory);
        memory = nullptr;
        memory64 = nullptr;
        code = nullptr;
        data = nullptr;
        heap = nullptr;
        end = nullptr;
    }
    
    allocated = 0;
    read_only_end = 0;
    stack_size = 0;
    
    std::memset(registers, 0, sizeof(vmregister) * 16);
    std::memset(&loaded, 0, sizeof(nnasm::instruction));
    
    running = false;
    stopped = false;
}

std::string vm::print_info() {
    using namespace std::chrono;
    std::stringstream ss{};
    if (!running) {
        ss << "Program done\n";
        duration program = end_time - start_time;
        ss << "Program took " << duration_cast<microseconds>(program).count() << "us\n";
        ss << "Executed " << steps << " instructions\n";
//         ss << "Spent " << load_time << "us loading instructions\n";
//         ss << "Spent " << execute_time << "us running instructions\n";
        ss << "At " << ((double) steps / duration_cast<seconds>(program).count()) << " instructions/sec\n";
    } else {
        if (stopped) {
            ss << "Program stopped\n";
        } else {
            ss << "Program running\n";
        }
    }
    ss << "Allocated: 0x" << std::hex << allocated << "\n";
    ss << " Read only memory: 0x" << read_only_end << "\n";
    ss << " Static memory: 0x" << (heap - data) << "\n";
    ss << " Heap size: 0x" << (((end - 1) - heap) - stack_size) << "\n";
    ss << " Stack size: 0x" << stack_size << "\n";
    ss << "\nRegisters:\n";
    for (u8 i = 0; i < 16; ++i) {
        ss << print_register(i) << "\n";
    }
    return ss.str();
}

std::string vm::print_register(u8 reg) {
    using namespace std::string_literals;
    std::stringstream ss{};
    ss << std::hex;
    static const std::string endings[] = {""s, "s"s, "8"s, "8s"s, "16"s, "16s"s, "32"s, "32s"s, "f"s, "d"s};
    if (reg == 14) {
        ss << "sf: \n";
        ss << "Zero: " << sf.zero << "\n";
        ss << "Check: " << sf.check << "\n";
    } else {
        std::string name = reg < 13 ? ("r"s + (char)('a' + reg)) : (reg == 13 ? "pc"s : "sp"s);
        for (u8 i = 0; i < sizeof(endings) / sizeof(std::string); ++i) {
            ss << name << endings[i] << ": ";
            switch (i) {
                case 0:
                    ss << registers[reg].qword << "\n";
                    break;
                case 1:
                    ss << registers[reg].sqword << "\n";
                    break;
                case 2:
                    ss << (u16) registers[reg].byte << "\n";
                    break;
                case 3:
                    ss << (i16) registers[reg].sbyte << "\n";
                    break;
                case 4: 
                    ss << registers[reg].word << "\n";
                    break;
                case 5:
                    ss << registers[reg].sword << "\n";
                    break;
                case 6:
                    ss << registers[reg].dword << "\n";
                    break;
                case 7:
                    ss << registers[reg].sdword << "\n";
                    break;
                case 8:
                    ss << registers[reg].fl << "\n";
                    break;
                case 9:
                    ss << registers[reg].db << "\n";
                    break;
            }
        }
    }
    return ss.str();
}

void vm::trap(int i) {
    // Do things eventually
    if (i > -1) {
        logger::error() << "Trapped " << i << logger::nend;
    }
}
