#include "vm/machine.h"

#include <iomanip>

#include "backend/nnasm.h"
#include "common/utils.h"

#define PROFILE false

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
        steps = 0;
        start_time = std::chrono::high_resolution_clock::now();
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
        end_time = std::chrono::high_resolution_clock::now();
    }
}

#define FIX_UNSIGNED(i) (ops_val[i].u & (u64)(-((op_sizes[i] << 3) != 0)) & ((u64)(-1) >> (64 - (op_sizes[i] << 3))))

#define FIX_SIGNED(i) (((((((1 << ((op_sizes[i] << 3) - 1)) & ops_val[i].s) == 0) -1) << ((op_sizes[i] << 3) - 1)) | ops_val[i].s))

void virtualmachine::step() {
    using namespace nnasm;
    using namespace std::chrono;
    
    // BEGIN Load instruction
#if PROFILE
    auto begin_load = std::chrono::high_resolution_clock::now();
#endif
    
    instr_hdr instr;
    std::memcpy(&instr, memory + pc.u, sizeof(instr_hdr));
    pc.u += sizeof(instr_hdr);
    vmregister ops_val[3] { {0}, {0}, {0} };
    vmregister* ops[3] {&ops_val[0], &ops_val[1], &ops_val[2]};
    u8 op_sizes[3] {0, 0, 0};
    
    for (u8 i = 0; i < instr.operands; ++i) {
        opertype op;
        switch (i) {
            case 0: op = (opertype) instr.op1type; break;
            case 1: op = (opertype) instr.op2type; break;
            case 2: op = (opertype) instr.op3type; break;
        }
        
        if (op == opertype::REG) {
            reg_hdr reg;
            std::memcpy(&reg, memory + pc.u, sizeof(reg_hdr));
            pc.u += sizeof(reg_hdr);
            u8 size = op_sizes[i] = 1 << reg.len;
            if (reg.floating) {
                ops[i] = &floating_registers[reg.reg];
            } else {
                ops[i] = &general_registers[reg.reg];
            }
            std::memcpy(&ops_val[i], ops[i], size);
        } else if (op == opertype::VAL) {
            imm_hdr imm;
            std::memcpy(&imm, memory + pc.u, sizeof(imm_hdr));
            pc.u += sizeof(imm_hdr);
            u8 size = op_sizes[i] = 1 << imm.len;
            std::memcpy(&ops_val[i], memory + pc.u, size);
            pc.u += size;
        } else { // Mem
            mem_hdr mem;
            std::memcpy(&mem, memory + pc.u, sizeof(mem_hdr));
            pc.u += sizeof(mem_hdr);
            u64 loc = 0;
            op_sizes[i] = 1 << mem.len;
            if (mem.reg) {
                reg_hdr reg;
                std::memcpy(&reg, memory + pc.u, sizeof(reg_hdr));
                pc.u += sizeof(reg_hdr);
                if (reg.floating) {
                    loc = floating_registers[reg.reg].u;
                } else {
                    loc = general_registers[reg.reg].u;
                }
            } else {
                u8 imm_size = 1 << (mem.imm_len);
                std::memcpy(&loc, memory + pc.u, imm_size);
                pc.u += imm_size;
            }
            
            if (mem.dis_type) {
                u64 off = 0;
                switch ((opertype) mem.dis_type) {
                    case opertype::REG: {
                        reg_hdr reg;
                        std::memcpy(&reg, memory + pc.u, sizeof(reg_hdr));
                        pc.u += sizeof(reg_hdr);
                        if (reg.floating) {
                            off = floating_registers[reg.reg].u;
                        } else {
                            off = general_registers[reg.reg].u;
                        }
                    }
                    default: {
                        u8 imm_size = 1 << (mem.imm_len);
                        std::memcpy(&off, memory + pc.u, imm_size);
                        pc.u += imm_size;
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
            std::memcpy(&ops_val[i], ops[i], op_sizes[i]);
        }
    }
    // END Load instruction
#if PROFILE
    loading += duration_cast<nanoseconds>(std::chrono::high_resolution_clock::now() - begin_load).count();
    
    auto begin_execute = std::chrono::high_resolution_clock::now();
#endif
    // BEGIN Execute instruction
    switch ((opcode) instr.code) {
        case opcode::NOP: {
            break;
        }
        case opcode::LOAD: {
            u8* from = memory + ops_val[0].u;
            if (from + op_sizes[1] > end) {
                trap(vmtraps::illegal_read);
                return;
            }
            std::memcpy(ops[1], from, op_sizes[1]);
            break;
        }
        case opcode::STOR: {
            u8* to = memory + ops_val[1].u;
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
            u8* from = memory + ops_val[0].u;
            u8* to = memory + ops_val[1].u;
            u64 amount = ops_val[2].u;
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
            u8* to = memory + ops_val[0].u;
            u64 amount = ops_val[1].u;
            if (to + amount > end || to < data) {
                trap(vmtraps::illegal_write);
                return;
            }
            std::memset(to, 0, amount);
            break;
        }
        case opcode::SET: {
            u8 val = (u8) ops_val[0].u;
            u8* to = memory + ops_val[1].u;
            u64 amount = ops_val[2].u;
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
            sf.check = ops_val[0].u == 0;
            break;
        }
        case opcode::CNZR: {
            sfr.u = 0;
            sf.check = ops_val[0].u != 0;
            break;
        }
        case opcode::CEQ: {
            sfr.u = 0;
            sf.check = ops_val[0].u == ops_val[1].u;
            break;
        }
        case opcode::CNEQ: {
            sfr.u = 0;
            sf.check = ops_val[0].u != ops_val[1].u;
            break;
        }
        case opcode::CBS: {
            sfr.u = 0;
            sf.check = (ops_val[0].u & (1ull << ops_val[1].u));
            break;
        }
        case opcode::CBNS: {
            sfr.u = 0;
            sf.check = (ops_val[0].u & (1ull << ops_val[1].u)) == 0;
            break;
        }
        
        case opcode::CLT: {
            sfr.u = 0;
            sf.check = ops_val[0].u < ops_val[1].u;
            break;
        }
        case opcode::SCLT: {
            sfr.u = 0;
            sf.check = FIX_SIGNED(0) < FIX_SIGNED(1);
            break;
        }
        case opcode::FCLT: {
            sfr.u = 0;
            sf.check = ops_val[0].f < ops_val[1].f;
            break;
        }
        case opcode::DCLT: {
            sfr.u = 0;
            sf.check = ops_val[0].d < ops_val[1].d;
            break;
        }
        case opcode::CLE: {
            sfr.u = 0;
            sf.check = ops_val[0].u <= ops_val[1].u;
            break;
        }
        case opcode::SCLE: {
            sfr.u = 0;
            sf.check = FIX_SIGNED(0) <= FIX_SIGNED(1);
            break;
        }
        case opcode::FCLE: {
            sfr.u = 0;
            sf.check = ops_val[0].f <= ops_val[1].f;
            break;
        }
        case opcode::DCLE: {
            sfr.u = 0;
            sf.check = ops_val[0].d <= ops_val[1].d;
            break;
        }
        
        case opcode::CGT: {
            sfr.u = 0;
            sf.check = ops_val[0].u > ops_val[1].u;
            break;
        }
        case opcode::SCGT: {
            sfr.u = 0;
            sf.check = FIX_SIGNED(0) > FIX_SIGNED(1);
            break;
        }
        case opcode::FCGT: {
            sfr.u = 0;
            sf.check = ops_val[0].f > ops_val[1].f;
            break;
        }
        case opcode::DCGT: {
            sfr.u = 0;
            sf.check = ops_val[0].d > ops_val[1].d;
            break;
        }
        case opcode::CGE: {
            sfr.u = 0;
            sf.check = ops_val[0].u >= ops_val[1].u;
            break;
        }
        case opcode::SCGE: {
            sfr.u = 0;
            sf.check = FIX_SIGNED(0) >= FIX_SIGNED(1);
            break;
        }
        case opcode::FCGE: {
            sfr.u = 0;
            sf.check = ops_val[0].f >= ops_val[1].f;
            break;
        }
        case opcode::DCGE: {
            sfr.u = 0;
            sf.check = ops_val[0].d >= ops_val[1].d;
            break;
        }
        
        case opcode::JMP: {
            u64 to = ops_val[0].u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            pc.u = to;
            break;
        }
        case opcode::JMPR: {
            u64 to = pc.u + ops_val[0].u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            pc.u = to;
            break;
        }
        case opcode::SJMPR: {
            u64 to = pc.s + ops_val[0].u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            pc.u = to;
            break;
        }
        case opcode::JCH: {
            u64 to = ops_val[0].u;
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
            u64 to = ops_val[0].u;
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
                if (sp.u - size < allocated - stack_size) {
                    trap(vmtraps::stack_overflow);
                    return;
                }
                sp.u -= size;
                std::memcpy(memory + sp.u, ops[0], size);
            } else {
                u8 size = ops_val[1].u;
                u8* from = memory + ops_val[0].u;
                if (sp.u - size < allocated - stack_size) {
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
                u8 size = ops_val[0].u;
                if (sp.u + size > allocated) {
                    trap(vmtraps::stack_underflow);
                    return;
                }
                sp.u += size;
            }
            break;
        }
        case opcode::BTIN: {
            switch (ops_val[0].u) {
                default: 
                    trap(vmtraps::illegal_btin);
                    return;
            }
            break;
        }
        case opcode::CALL: {
            u64 to = ops_val[0].u;
            if (memory + to > data) {
                trap(vmtraps::illegal_jump);
                return;
            }
            if (sp.u - sizeof(vmregister) < allocated - stack_size) {
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
            u64 u = (u64) FIX_SIGNED(0);
            std::memcpy(ops[instr.operands - 1], &u, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::CSTF: {
            ops[instr.operands - 1]->f = (float) FIX_SIGNED(0);
            break;
        }
        case opcode::CSTD: {
            ops[instr.operands - 1]->d = (double) FIX_SIGNED(0);
            break;
        }
        case opcode::CUTS: {
            i64 s = ops_val[0].u;
            std::memcpy(ops[instr.operands - 1], &s, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::CUTF: {
            ops[instr.operands - 1]->f = (float) ops_val[0].u;
            break;
        }
        case opcode::CUTD: {
            ops[instr.operands - 1]->d = (double) ops_val[0].u;
            break;
        }
        case opcode::CFTS: {
            i64 s = ops_val[0].f;
            std::memcpy(ops[instr.operands - 1], &s, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::CFTU: {
            u64 u = ops_val[0].f;
            std::memcpy(ops[instr.operands - 1], &u, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::CFTD: {
            ops[instr.operands - 1]->d = (double) ops[0]->f;
            break;
        }
        case opcode::CDTS: {
            i64 s = ops_val[0].d;
            std::memcpy(ops[instr.operands - 1], &s, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::CDTU: {
            u64 u = ops_val[0].d;
            std::memcpy(ops[instr.operands - 1], &u, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::CDTF: {
            ops[instr.operands - 1]->f = (float) ops[0]->d;
            break;
        }
        
        case opcode::ADD: {
            u64 res = ops_val[0].u + ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SADD: {
            i64 res = FIX_SIGNED(0) + FIX_SIGNED(1);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::FADD: {
            ops[instr.operands - 1]->f = ops_val[0].f + ops_val[1].f;
            break;
        }
        case opcode::DADD: {
            ops[instr.operands - 1]->d = ops_val[0].d + ops_val[1].d;
            break;
        }
        case opcode::INC: {
            u64 res = ops_val[0].u + 1;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SINC: {
            i64 res = FIX_SIGNED(0) + 1;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SUB: {
            u64 res = ops_val[0].u - ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SSUB: {
            i64 res = FIX_SIGNED(0) - FIX_SIGNED(1);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::FSUB: {
            ops[instr.operands - 1]->f = ops_val[0].f - ops_val[1].f;
            break;
        }
        case opcode::DSUB: {
            ops[instr.operands - 1]->d = ops_val[0].d - ops_val[1].d;
            break;
        }
        case opcode::DEC: {
            u64 res = ops_val[0].u - 1;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SDEC: {
            i64 res = FIX_SIGNED(0) - 1;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::MUL: {
            u64 res = ops_val[0].u * ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SMUL: {
            i64 res = FIX_SIGNED(0) * FIX_SIGNED(1);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::FMUL: {
            ops[instr.operands - 1]->f = ops_val[0].f * ops_val[1].f;
            break;
        }
        case opcode::DMUL: {
            ops[instr.operands - 1]->d = ops_val[0].d * ops_val[1].d;
            break;
        }
        case opcode::DIV: {
            u64 res = ops_val[1].u / ops_val[0].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SDIV: {
            i64 res = FIX_SIGNED(1) / FIX_SIGNED(0);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::FDIV: {
            ops[instr.operands - 1]->f = ops_val[1].f / ops_val[0].f;
            break;
        }
        case opcode::DDIV: {
            ops[instr.operands - 1]->d = ops_val[1].d / ops_val[0].d;
            break;
        }
        case opcode::MOD: {
            u64 res = ops_val[0].u % ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SMOD: {
            i64 res = FIX_SIGNED(0) % FIX_SIGNED(1);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SABS: {
            i64 res = std::abs(FIX_SIGNED(0));
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::FABS: {
            ops[instr.operands - 1]->f = std::abs(ops_val[0].f);
            break;
        }
        case opcode::DABS: {
            ops[instr.operands - 1]->d = std::abs(ops_val[0].d);
            break;
        }
        case opcode::SNEG: {
            i64 res = -FIX_SIGNED(0);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::FNEG: {
            ops[instr.operands - 1]->f = -ops_val[0].f;
            break;
        }
        case opcode::DNEG: {
            ops[instr.operands - 1]->d = -ops_val[0].d;
            break;
        }
        
        case opcode::SHR: {
            u64 res = ops_val[0].u >> ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SSHR: {
            i64 res = FIX_SIGNED(0) >> FIX_SIGNED(1);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SHL: {
            u64 res = ops_val[0].u << ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::SSHL: {
            i64 res = FIX_SIGNED(0) << FIX_SIGNED(1);
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::RTR: {
            u64 res = (ops_val[0].u >> ops_val[1].u) | (ops_val[0].u << (op_sizes[0] - ops_val[1].u));
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::RTL: {
            u64 res = (ops_val[0].u << ops_val[1].u) | (ops_val[0].u >> (op_sizes[0] - ops_val[1].u));
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        
        case opcode::AND: {
            u64 res = ops_val[0].u & ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::OR: {
            u64 res = ops_val[0].u | ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::XOR: {
            u64 res = ops_val[0].u ^ ops_val[1].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        case opcode::NOT: {
            u64 res = ~ops_val[0].u;
            std::memcpy(ops[instr.operands - 1], &res, op_sizes[instr.operands - 1]);
            break;
        }
        default: {
            trap(vmtraps::illegal_instruction);
            break;
        }
    }
    // END Execute instruction
#if PROFILE
    executing += duration_cast<nanoseconds>(std::chrono::high_resolution_clock::now() - begin_execute).count();
#endif
    ++steps;
}

void virtualmachine::pause() {
    to_pause = true;
}

void virtualmachine::stop() {
    ended = true;
}

std::string virtualmachine::print_info() {
    using namespace std::chrono;
    std::stringstream ss{};
    if (ended) {
        ss << "Program done\n";
        auto time = end_time - start_time;
        ss << "Program took " << duration_cast<microseconds>(time).count() << "us\n";
        ss << "Executed " << steps << " instructions @ " << ((double) steps / (double) duration_cast<seconds>(time).count()) << " instructions/sec\n";
#if PROFILE
        ss << "Took " << loading << "ns loading instructions\n";
        ss << "Took " << executing << "ns executing instructions\n";
#endif
    } else if (paused) {
        ss << "Program paused\n";
    } else {
        ss << "Program running\n";
    }
    
    ss << std::hex << "Allocated 0x" << allocated << " bytes\n";
    ss << " Read only memory: 0x" << read_only_end << " bytes\n";
    ss << " Static memory: 0x" << (heap - data) << " bytes\n";
    ss << " Heap size: 0x" <<  (((end - 1) - heap) - stack_size) << " bytes\n";
    ss << " Stack size: 0x" << stack_size << " bytes\n";
    return ss.str();
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
    if (signal > 0) {
        logger::error() << "Got signal " << signal << logger::nend;
    } else {
        logger::info() << "Got signal " << signal << logger::nend;
    }
}

