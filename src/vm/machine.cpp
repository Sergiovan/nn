#include "vm/machine.h"

#include <iomanip>

#include "backend/nnasm.h"
#include "common/utils.h"


virtualmachine::virtualmachine() {
    
}

virtualmachine::~virtualmachine() {
    if (memory) {
        delete [] memory;
    }
}

void virtualmachine::load(u8* program, u64 size) {
    clear();
    
    nnexe_header* hdr = reinterpret_cast<nnexe_header*>(program);
    // TODO Header check
    u64 code_start = hdr->code_start;
    u64 data_start = hdr->data_start;
    u64 file_size  = hdr->size;
    u64 initial_alloc = hdr->initial;
    
    allocate(file_size + initial_alloc);
    
    code = memory + code_start;
    read_only_end = data_start;
    data = memory + data_start;
    heap = memory + file_size;
    stack_size = std::min(initial_alloc >> 1, (u64) 1 << 23); // Half the space or 4MB
    
    std::memcpy(memory, program, size);
}

void virtualmachine::run() {
    started = true;
    paused = false;
    if (ended) {
        pc.u = code - memory; // Start of code
        sp.u = allocated;
        ended = false;
    }
    
    while (!ended && !to_pause) {
        step();
    }
    
    if (to_pause) {
        to_pause = false;
        paused = true;
    }
    if (ended) {
        started = false;
    }
}

void virtualmachine::step() {
    using namespace nnasm;
    
    instr_hdr instr = read_from_pc<instr_hdr>();
    vmregister ops_temp[3] { {0}, {0}, {0} };
    vmregister* ops[3] {&ops_temp[0], &ops_temp[1], &ops_temp[2]};
    u8 op_sizes[3] {0, 0, 0};
    
    for (u8 i = 0; i < instr.operands; ++i) {
        opertype op;
        switch(i) {
            case 0: op = (opertype) instr.op1type; break;
            case 1: op = (opertype) instr.op2type; break;
            case 2: op = (opertype) instr.op3type; break;
        }
        
        if (op == opertype::REG) {
            reg_hdr reg = read_from_pc<reg_hdr>();
            if (reg.floating) {
                ops[i] = &floating_registers[reg.reg];
            } else {
                ops[i] = &general_registers[reg.reg];
            }
            op_sizes[i] = 1 << (3 + reg.len);
        } else if (op == opertype::VAL) {
            imm_hdr imm = read_from_pc<imm_hdr>();
            op_sizes[i] = 1 << (3 + imm.len);
            switch((operlen) imm.len) {
                case operlen::_8:  ops_temp[i].u = read_from_pc<u8>(); break;
                case operlen::_16: ops_temp[i].u = read_from_pc<u16>(); break;
                case operlen::_32: ops_temp[i].u = read_from_pc<u32>(); break;
                case operlen::_64: ops_temp[i].u = read_from_pc<u64>(); break;
            }
        } else { // Mem
            mem_hdr mem = read_from_pc<mem_hdr>();
            u64 loc = 0;
            op_sizes[i] = 1 << (3 + mem.len);
            if (mem.reg) {
                reg_hdr reg = read_from_pc<reg_hdr>();
                if (reg.floating) {
                    loc = floating_registers[reg.reg].u;
                } else {
                    loc = general_registers[reg.reg].u;
                }
            } else {
                switch ((operlen) mem.imm_len) {
                    case operlen::_8:  loc = read_from_pc<u8>(); break;
                    case operlen::_16: loc = read_from_pc<u16>(); break;
                    case operlen::_32: loc = read_from_pc<u32>(); break;
                    case operlen::_64: loc = read_from_pc<u64>(); break;
                }
            }
            
            if (mem.dis_type) {
                u64 off = 0;
                switch ((opertype) mem.dis_type) {
                    case opertype::REG: {
                        reg_hdr reg = read_from_pc<reg_hdr>();
                        if (reg.floating) {
                            off = floating_registers[reg.reg].u;
                        } else {
                            off = general_registers[reg.reg].u;
                        }
                    }
                    case opertype::VAL: {
                        switch ((operlen) mem.imm_len) {
                            case operlen::_8:  off = read_from_pc<u8>(); break;
                            case operlen::_16: off = read_from_pc<u16>(); break;
                            case operlen::_32: off = read_from_pc<u32>(); break;
                            case operlen::_64: off = read_from_pc<u64>(); break;
                        }
                    }
                }
                if (mem.dis_signed) {
                    loc -= off;
                } else {
                    loc += off;
                }
            }
            if (memory + loc + op_sizes[i] > end) {
                trap(vmtraps::illegal_read);
                return;
            }
            
            ops[i] = reinterpret_cast<vmregister*>(memory + loc);
        }
    }
    
    switch ((opcode) instr.code) {
        case opcode::NOP: {
            break;
        }
        case opcode::LOAD: {
            u8* from = memory + ops[0]->u;
            if (from + op_sizes[1] > end) {
                trap(vmtraps::illegal_read);
                return;
            }
            std::memcpy(ops[1], from, op_sizes[1]);
            break;
        }
        case opcode::STOR: {
            u8* to = memory + ops[1]->u;
            if (to + op_sizes[0] > end || to < data) {
                trap(vmtraps::illegal_write);
                return;
            }
            std::memcpy(to, ops[0], op_sizes[0]);
            break;
        }
        case opcode::MOV: {
            std::memcpy(ops[1], ops[0], op_sizes[1]);
            break;
        }
        case opcode::CPY: {
            u8* from = memory + ops[0]->u;
            u8* to = memory + ops[1]->u;
            u64 amount = ops[2]->u;
            if (from + amount > end) {
                trap(vmtraps::illegal_read);
                return;
            }
            if (to + amount > end || to < data) {
                trap(vmtraps::illegal_write);
                return;
            }
            if (to + amount > from) {
                std::memmove(to, from, amount);
            } else {
                std::memcpy(to, from, amount);
            }
            break;
        }
        case opcode::ZRO: {
            u8* to = memory + ops[0]->u;
            u64 amount = ops[1]->u;
            if (to + amount > end || to < data) {
                trap(vmtraps::illegal_write);
                return;
            }
            std::memset(to, 0, amount);
            break;
        }
        case opcode::SET: {
            u8 val = (u8) ops[0]->u;
            u8* to = memory + ops[1]->u;
            u64 amount = ops[2]->u;
            if (to + amount > end || to < data) {
                trap(vmtraps::illegal_write);
                return;
            }
            std::memset(to, val, amount);
            break;
        }
        case opcode::BRK: {
            trap(vmtraps::break_trap);
            pause();
            break;
        }
        case opcode::HLT: {
            trap(vmtraps::halt);
            stop();
            break;
        }
        case opcode::CZRO: {
            sfr.u = 0;
            sf.check = ops[0]->u == 0;
            break;
        }
        case opcode::CNZR: {
            sfr.u = 0;
            sf.check = ops[0]->u != 0;
            break;
        }
        case opcode::CEQ: {
            sfr.u = 0;
            sf.check = ops[0]->u == ops[1]->u;
            break;
        }
        case opcode::CNEQ: {
            sfr.u = 0;
            sf.check = ops[0]->u != ops[1]->u;
            break;
        }
        case opcode::CBS: {
            sfr.u = 0;
            sf.check = (ops[0]->u & (1ull << ops[1]->u));
            break;
        }
        case opcode::CBNS: {
            sfr.u = 0;
            sf.check = (ops[0]->u & (1ull << ops[1]->u)) == 0;
            break;
        }
        
        case opcode::CLT: {
            sfr.u = 0;
            sf.check = ops[0]->u < ops[1]->u;
            break;
        }
        case opcode::SCLT: {
            sfr.u = 0;
            sf.check = ops[0]->s < ops[1]->s;
            break;
        }
        case opcode::FCLT: {
            sfr.u = 0;
            sf.check = ops[0]->f < ops[1]->f;
            break;
        }
        case opcode::DCLT: {
            sfr.u = 0;
            sf.check = ops[0]->d < ops[1]->d;
            break;
        }
        case opcode::CLE: {
            sfr.u = 0;
            sf.check = ops[0]->u <= ops[1]->u;
            break;
        }
        case opcode::SCLE: {
            sfr.u = 0;
            sf.check = ops[0]->s <= ops[1]->s;
            break;
        }
        case opcode::FCLE: {
            sfr.u = 0;
            sf.check = ops[0]->f <= ops[1]->f;
            break;
        }
        case opcode::DCLE: {
            sfr.u = 0;
            sf.check = ops[0]->d <= ops[1]->d;
            break;
        }
        
        case opcode::CGT: {
            sfr.u = 0;
            sf.check = ops[0]->u > ops[1]->u;
            break;
        }
        case opcode::SCGT: {
            sfr.u = 0;
            sf.check = ops[0]->s > ops[1]->s;
            break;
        }
        case opcode::FCGT: {
            sfr.u = 0;
            sf.check = ops[0]->f > ops[1]->f;
            break;
        }
        case opcode::DCGT: {
            sfr.u = 0;
            sf.check = ops[0]->d > ops[1]->d;
            break;
        }
        case opcode::CGE: {
            sfr.u = 0;
            sf.check = ops[0]->u >= ops[1]->u;
            break;
        }
        case opcode::SCGE: {
            sfr.u = 0;
            sf.check = ops[0]->s >= ops[1]->s;
            break;
        }
        case opcode::FCGE: {
            sfr.u = 0;
            sf.check = ops[0]->f >= ops[1]->f;
            break;
        }
        case opcode::DCGE: {
            sfr.u = 0;
            sf.check = ops[0]->d >= ops[1]->d;
            break;
        }
        
        case opcode::JMP: {
            u64 to = ops[0]->u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            pc.u = to;
            break;
        }
        case opcode::JMPR: {
            u64 to = pc.u + ops[0]->u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            pc.u = to;
            break;
        }
        case opcode::SJMPR: {
            u64 to = pc.s + ops[0]->u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            pc.u = to;
            break;
        }
        case opcode::JCH: {
            u64 to = ops[0]->u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            if (sf.check) {
                pc.u = to;
            }
            break;
        }
        case opcode::JNCH: {
            u64 to = ops[0]->u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            if (!sf.check) {
                pc.u = to;
            }
            break;
        }
        case opcode::PUSH: {
            if (instr.operands == 1) {
                u8 size = op_sizes[0];
                if (sp.u - size > allocated - stack_size) {
                    trap(vmtraps::stack_overflow);
                    return;
                }
                sp.u -= size;
                std::memcpy(memory + sp.u, ops[0], size);
            } else {
                u8 size = ops[1]->u;
                u8* from = memory + ops[0]->u;
                if (sp.u - size > allocated - stack_size) {
                    trap(vmtraps::stack_overflow);
                    return;
                }
                if (from + size > end) {
                    trap(vmtraps::illegal_read);
                }
                sp.u -= size;
                std::memcpy(memory + sp.u, from, size);
            }
            break;
        }
        case opcode::POP: {
            if (instr.op1type == (u8) opertype::REG) {
                u8 size = op_sizes[0];
                if (sp.u + size > allocated) {
                    trap(vmtraps::stack_underflow);
                    return;
                }
                std::memcpy(&ops[0], memory + sp.u, size);
                sp.u += size;
            } else {
                u8 size = ops[0]->u;
                if (sp.u + size > allocated) {
                    trap(vmtraps::stack_underflow);
                    return;
                }
                sp.u += size;
            }
            break;
        }
        case opcode::BTIN: {
            switch (ops[0]->u) {
                default: 
                    trap(vmtraps::illegal_btin);
                    return;
            }
            break;
        }
        case opcode::CALL: {
            u64 to = pc.u + ops[0]->u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            if (sp.u - sizeof(vmregister) > allocated - stack_size) {
                trap(vmtraps::stack_overflow);
                return;
            }
            sp.u -= sizeof(vmregister);
            std::memcpy(memory + sp.u, &pc, sizeof(vmregister));
            pc.u = to;
            break;
        }
        case opcode::RET: {
            if (sp.u + sizeof(vmregister) > allocated) {
                trap(vmtraps::stack_underflow);
                return;
            }
            std::memcpy(&pc.u, memory + sp.u, sizeof(vmregister));
            sp.u += sizeof(vmregister);
            break;
        }
        case opcode::CSTU: {
            if (instr.operands == 1) {
                ops[0]->u = (u64) ops[0]->s;
            } else {
                ops[1]->u = (u64) ops[0]->s;
            }
            break;
        }
        case opcode::CSTF: {
            if (instr.operands == 1) {
                ops[0]->f = (float) ops[0]->s;
            } else {
                ops[1]->f = (float) ops[0]->s;
            }
            break;
        }
        case opcode::CSTD: {
            if (instr.operands == 1) {
                ops[0]->d = (double) ops[0]->s;
            } else {
                ops[1]->d = (double) ops[0]->s;
            }
            break;
        }
        case opcode::CUTS: {
            if (instr.operands == 1) {
                ops[0]->s = (i64) ops[0]->u;
            } else {
                ops[1]->s = (i64) ops[0]->u;
            }
            break;
        }
        case opcode::CUTF: {
            if (instr.operands == 1) {
                ops[0]->f = (float) ops[0]->u;
            } else {
                ops[1]->f = (float) ops[0]->u;
            }
            break;
        }
        case opcode::CUTD: {
            if (instr.operands == 1) {
                ops[0]->d = (double) ops[0]->u;
            } else {
                ops[1]->d = (double) ops[0]->u;
            }
            break;
        }
        case opcode::CFTS: {
            if (instr.operands == 1) {
                ops[0]->s = (i64) ops[0]->f;
            } else {
                ops[1]->s = (i64) ops[0]->f;
            }
            break;
        }
        case opcode::CFTU: {
            if (instr.operands == 1) {
                ops[0]->u = (u64) ops[0]->f;
            } else {
                ops[1]->u = (u64) ops[0]->f;
            }
            break;
        }
        case opcode::CFTD: {
            if (instr.operands == 1) {
                ops[0]->d = (double) ops[0]->f;
            } else {
                ops[1]->d = (double) ops[0]->f;
            }
            break;
        }
        case opcode::CDTS: {
            if (instr.operands == 1) {
                ops[0]->s = (i64) ops[0]->d;
            } else {
                ops[1]->s = (i64) ops[0]->d;
            }
            break;
        }
        case opcode::CDTU: {
            if (instr.operands == 1) {
                ops[0]->u = (u64) ops[0]->d;
            } else {
                ops[1]->u = (u64) ops[0]->d;
            }
            break;
        }
        case opcode::CDTF: {
            if (instr.operands == 1) {
                ops[0]->f = (float) ops[0]->d;
            } else {
                ops[1]->f = (float) ops[0]->d;
            }
            break;
        }
        
        case opcode::ADD: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u + ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u + ops[1]->u;
            }
            break;
        }
        case opcode::SADD: {
            if (instr.operands == 2) {
                ops[1]->s = ops[0]->s + ops[1]->s;
            } else {
                ops[2]->s = ops[0]->s + ops[1]->s;
            }
            break;
        }
        case opcode::FADD: {
            if (instr.operands == 2) {
                ops[1]->f = ops[0]->f + ops[1]->f;
            } else {
                ops[2]->f = ops[0]->f + ops[1]->f;
            }
            break;
        }
        case opcode::DADD: {
            if (instr.operands == 2) {
                ops[1]->d = ops[0]->d + ops[1]->d;
            } else {
                ops[2]->d = ops[0]->d + ops[1]->d;
            }
            break;
        }
        case opcode::INC: {
            if (instr.operands == 1) {
                ++ops[0]->u;
            } else {
                ops[1]->u = ops[0]->u + 1;
            }
            break;
        }
        case opcode::SINC: {
            if (instr.operands == 1) {
                ++ops[0]->s;
            } else {
                ops[1]->s = ops[0]->s + 1;
            }
            break;
        }
        case opcode::SUB: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u - ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u - ops[1]->u;
            }
            break;
        }
        case opcode::SSUB: {
            if (instr.operands == 2) {
                ops[1]->s = ops[0]->s - ops[1]->s;
            } else {
                ops[2]->s = ops[0]->s - ops[1]->s;
            }
            break;
        }
        case opcode::FSUB: {
            if (instr.operands == 2) {
                ops[1]->f = ops[0]->f - ops[1]->f;
            } else {
                ops[2]->f = ops[0]->f - ops[1]->f;
            }
            break;
        }
        case opcode::DSUB: {
            if (instr.operands == 2) {
                ops[1]->d = ops[0]->d - ops[1]->d;
            } else {
                ops[2]->d = ops[0]->d - ops[1]->d;
            }
            break;
        }
        case opcode::DEC: {
            if (instr.operands == 1) {
                --ops[0]->u;
            } else {
                ops[1]->u = ops[0]->u - 1;
            }
            break;
        }
        case opcode::SDEC: {
            if (instr.operands == 1) {
                --ops[0]->s;
            } else {
                ops[1]->s = ops[0]->s - 1;
            }
            break;
        }
        case opcode::MUL: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u * ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u * ops[1]->u;
            }
            break;
        }
        case opcode::SMUL: {
            if (instr.operands == 2) {
                ops[1]->s = ops[0]->s * ops[1]->s;
            } else {
                ops[2]->s = ops[0]->s * ops[1]->s;
            }
            break;
        }
        case opcode::FMUL: {
            if (instr.operands == 2) {
                ops[1]->f = ops[0]->f * ops[1]->f;
            } else {
                ops[2]->f = ops[0]->f * ops[1]->f;
            }
            break;
        }
        case opcode::DMUL: {
            if (instr.operands == 2) {
                ops[1]->d = ops[0]->d * ops[1]->d;
            } else {
                ops[2]->d = ops[0]->d * ops[1]->d;
            }
            break;
        }
        case opcode::DIV: {
            if (instr.operands == 2) {
                ops[1]->u = ops[1]->u / ops[0]->u;
            } else {
                ops[2]->u = ops[1]->u / ops[0]->u;
            }
            break;
        }
        case opcode::SDIV: {
            if (instr.operands == 2) {
                ops[1]->s = ops[1]->s / ops[0]->s;
            } else {
                ops[2]->s = ops[1]->s / ops[0]->s;
            }
            break;
        }
        case opcode::FDIV: {
            if (instr.operands == 2) {
                ops[1]->f = ops[1]->f / ops[0]->f;
            } else {
                ops[2]->f = ops[1]->f / ops[0]->f;
            }
            break;
        }
        case opcode::DDIV: {
            if (instr.operands == 2) {
                ops[1]->d = ops[1]->d / ops[0]->d;
            } else {
                ops[2]->d = ops[1]->d / ops[0]->d;
            }
            break;
        }
        case opcode::MOD: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u % ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u % ops[1]->u;
            }
            break;
        }
        case opcode::SMOD: {
            if (instr.operands == 2) {
                ops[1]->s = ops[0]->s % ops[1]->s;
            } else {
                ops[2]->s = ops[0]->s % ops[1]->s;
            }
            break;
        }
        case opcode::SABS: {
            if (instr.operands == 1) {
                ops[0]->s = std::abs(ops[0]->s);
            } else {
                ops[1]->s = std::abs(ops[0]->s);
            }
            break;
        }
        case opcode::FABS: {
            if (instr.operands == 1) {
                ops[0]->f = std::abs(ops[0]->f);
            } else {
                ops[1]->f = std::abs(ops[0]->f);
            }
            break;
        }
        case opcode::DABS: {
            if (instr.operands == 1) {
                ops[0]->d = std::abs(ops[0]->d);
            } else {
                ops[1]->d = std::abs(ops[0]->d);
            }
            break;
        }
        case opcode::SNEG: {
            if (instr.operands == 1) {
                ops[0]->s = -ops[0]->s;
            } else {
                ops[1]->s = -ops[0]->s;
            }
            break;
        }
        case opcode::FNEG: {
            if (instr.operands == 1) {
                ops[0]->f = -ops[0]->f;
            } else {
                ops[1]->f = -ops[0]->f;
            }
            break;
        }
        case opcode::DNEG: {
            if (instr.operands == 1) {
                ops[0]->d = -ops[0]->d;
            } else {
                ops[1]->d = -ops[0]->d;
            }
            break;
        }
        
        case opcode::SHR: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u >> ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u >> ops[1]->u;
            }
            break;
        }
        case opcode::SSHR: {
            if (instr.operands == 2) {
                ops[1]->s = ops[0]->s >> ops[1]->s;
            } else {
                ops[2]->s = ops[0]->s >> ops[1]->s;
            }
            break;
        }
        case opcode::SHL: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u << ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u << ops[1]->u;
            }
            break;
        }
        case opcode::SSHL: {
            if (instr.operands == 2) {
                ops[1]->s = ops[0]->s << ops[1]->s;
            } else {
                ops[2]->s = ops[0]->s << ops[1]->s;
            }
            break;
        }
        case opcode::RTR: {
            if (instr.operands == 2) {
                ops[1]->u = (ops[0]->u >> ops[1]->u) | (ops[0]->u << (64 - ops[1]->u));
            } else {
                ops[2]->u = (ops[0]->u >> ops[1]->u) | (ops[0]->u << (64 - ops[1]->u));
            }
            break;
        }
        case opcode::RTL: {
            if (instr.operands == 2) {
                ops[1]->u = (ops[0]->u << ops[1]->u) | (ops[0]->u >> (64 - ops[1]->u));
            } else {
                ops[2]->u = (ops[0]->u << ops[1]->u) | (ops[0]->u >> (64 - ops[1]->u));
            }
            break;
        }
        
        case opcode::AND: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u & ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u & ops[1]->u;
            }
            break;
        }
        case opcode::OR: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u | ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u | ops[1]->u;
            }
            break;
        }
        case opcode::XOR: {
            if (instr.operands == 2) {
                ops[1]->u = ops[0]->u ^ ops[1]->u;
            } else {
                ops[2]->u = ops[0]->u ^ ops[1]->u;
            }
            break;
        }
        case opcode::NOT: {
            if (instr.operands == 2) {
                ops[0]->u = ~ops[0]->u;
            } else {
                ops[1]->u = ~ops[0]->u;
            }
            break;
        }
        default: {
            trap(vmtraps::illegal_instruction);
            break;
        }
    }
}

void virtualmachine::pause() {
    to_pause = true;
}

void virtualmachine::stop() {
    ended = true;
}

std::string virtualmachine::print_info() {
    return {};
}

std::string virtualmachine::print_registers() {
    std::stringstream ss{};
    static const char* general_names[] {
        "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
        "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
        "pc", "sf", "sp"
    };
    static const char* floating_names[] {
        "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
        "f9", "f10", "f11", "f12", "f13", "f14", "f15", "f16"
    };
    for (int i = 0; i < 19; ++i) {
        ss << general_names[i] << ": \n";
        ss << "   u: " << std::hex << general_registers[i].u << "\n";
        ss << "   s: " << std::dec << general_registers[i].s << "\n";
    }
    for (int i = 0; i < 16; ++i) {
        ss << floating_names[i] << ": \n";
        ss << "   f: " << floating_registers[i].f << "\n";
        ss << "   d: " << floating_registers[i].d << "\n";
    }
    
    return ss.str();
}

void virtualmachine::allocate(u64 amount) {
    if (memory) {
        u64 code_start = code - memory;
        u64 data_start = read_only_end;
        u64 file_size = heap - memory;
        
        u8* buff = new u8[allocated + amount + 8]; // TODO Catch out_of_memory
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy((buff + allocated + amount) - stack_size, memory + allocated - stack_size, stack_size);
        delete [] memory;
        memory = buff;
        sp.u += amount;
        allocated += amount;
        
        code = memory + code_start;
        read_only_end = data_start;
        data = memory + data_start;
        heap = memory + file_size;
        stack_size = std::min((allocated - file_size) >> 1, (u64) 1 << 23); // Half the space or 4MB
        end = memory + allocated + 1;
    } else {
        memory = new u8[amount + 8]; // TODO Catch out_of_memory
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void virtualmachine::resize(u64 amount) {
    if (memory) {        
        u64 code_start = code - memory;
        u64 data_start = read_only_end;
        u64 file_size = heap - memory;
        
        // TODO Check viability with heap size via allocator
        // TODO Check math? 
        
        u8* buff = new u8[amount + 8];
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy((buff + amount) - stack_size, memory + allocated - stack_size, stack_size);
        delete [] memory;
        memory = buff;
        sp.u += (amount - allocated);
        allocated = amount;
        
        code = memory + code_start;
        read_only_end = data_start;
        data = memory + data_start;
        heap = memory + file_size;
        stack_size = std::min((allocated - file_size) >> 1, (u64) 1 << 23); // Half the space or 4MB
        end = memory + allocated + 1;
    } else {
        memory = new u8[amount + 8]; // TODO Catch out_of_memory
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void virtualmachine::clear() {
    if (memory) {
        delete [] memory;
    }
    
    memory = code = data = heap = end = nullptr;
    allocated = read_only_end = stack_size = 0;
    
    std::memset(general_registers,  0, sizeof general_registers);
    std::memset(floating_registers, 0, sizeof floating_registers);
    
    started = to_pause = paused = false;
    ended = true;
}

void virtualmachine::trap(i64 signal) {
    // TODO Do something
    logger::error() << "Got signal " << signal << logger::nend;
}

