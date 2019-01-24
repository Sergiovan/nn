#pragma once

#include <chrono>
#include <cstring>
#include "common/asm.h"
#include "common/convenience.h"

class parser;

struct vmregister {
    union {
        u64 qword{0};
        i64 sqword;
        u8 byte;
        i8 sbyte;
        u16 word;
        i16 sword;
        u32 dword;
        i32 sdword;
        float fl;
        double db;
    };
};

struct vmflagsregister {
    u64 zero : 1;
    u64 check : 1;
    u64 reserved : 62;
};

class virtualmachine {
public:
    virtualmachine(parser& p);
    ~virtualmachine();
    
    virtualmachine(const virtualmachine&) = delete;
    virtualmachine(virtualmachine&&) = delete;
    virtualmachine& operator=(const virtualmachine&) = delete;
    virtualmachine& operator=(virtualmachine&&) = delete;
    
    void run();
    void step();
    void load_instruction();
    void execute_instruction();
    void stop();
    
    void load(u8* program, u64 size);
    void allocate(u64 amount);
    void resize(u64 amount);
    void clear();
    
    std::string print_info();
    std::string print_register(u8 reg);
private:
    vmregister registers[16]{};
    vmregister& ra = registers[0];
    vmregister& rb = registers[1];
    vmregister& rc = registers[2];
    vmregister& rd = registers[3];
    vmregister& re = registers[4];
    vmregister& rf = registers[5];
    vmregister& rg = registers[6];
    vmregister& rh = registers[7];
    vmregister& ri = registers[8];
    vmregister& rj = registers[9];
    vmregister& rk = registers[10];
    vmregister& rl = registers[11];
    vmregister& rm = registers[12];
    vmregister& pc = registers[13];
    vmflagsregister& sf = *reinterpret_cast<vmflagsregister*>(&registers[14]);
    vmregister& sp = registers[15];
    
    nnasm::instruction loaded{};
    
    u8* memory{nullptr}; // Owned
    u64* memory64{nullptr};
    u8* code{nullptr};
    u8* data{nullptr};
    u8* heap{nullptr};
    u8* end{nullptr};
    
    u64 allocated{0};
    u64 read_only_end{0};
    u64 stack_size{0};
    
    decltype(std::chrono::high_resolution_clock::now()) start_time{};
    decltype(std::chrono::high_resolution_clock::now()) end_time{};
    
    u64 load_time{0};
    u64 execute_time{0};
    
    
    u64 steps{};
    
    parser& p;

    bool running{false};
    bool stopped{false};
    
    void trap(int i);
    
    constexpr inline u64 absolute_address(i64 address) {
        return address < 0 ? (allocated + address) : address;
    }

    template <typename T>
    inline void push(T t) {
        u64 size = sizeof(T);
        if (sp.qword + size > stack_size) {
            trap(2);
            return;
        }
        sp.qword += size;
        std::memcpy(end - sp.qword, &t, size);
    }
    
    template <typename T>
    inline T pop() {
        u64 size = sizeof(T);
        if (sp.qword - size > stack_size) {
            trap(3);
            return {};
        }
        T buff;
        std::memcpy(&buff, end - sp.qword, size);
        sp.qword -= size;
        return buff;
    }
    
    template <typename T, typename U, typename V, typename W>
    inline void ins0op(W& w, V& v, U& u, T& t) {
        t(w, v, u);
    }

    using anonop = decltype(loaded.code.ops[0]);
    
    template <typename F, typename... Args>
    inline void insopnderef(vmregister& reg, anonop& op, F& f, Args&... args) {
        using namespace nnasm::op;
        if (op.valtype == ASM_TYPE_BYTE) {
            auto& opval = reg.byte;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SBYTE) {
            auto& opval = reg.sbyte;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_WORD) {
            auto& opval = reg.word;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SWORD) {
            auto& opval = reg.sword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_DWORD) {
            auto& opval = reg.dword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SDWORD) {
            auto& opval = reg.sdword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_QWORD) {
            auto& opval = reg.qword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SQWORD) {
            auto& opval = reg.sqword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_FLOAT) {
            auto& opval = reg.fl;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_DOUBLE) {
            auto& opval = reg.db;
            f(opval, args...);
        }
    }
    
    template <typename F, typename... Args>
    inline void insopnderef_int(vmregister& reg, anonop& op, F& f, Args&... args) {
        using namespace nnasm::op;
        if (op.valtype == ASM_TYPE_BYTE) {
            auto& opval = reg.byte;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SBYTE) {
            auto& opval = reg.sbyte;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_WORD) {
            auto& opval = reg.word;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SWORD) {
            auto& opval = reg.sword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_DWORD) {
            auto& opval = reg.dword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SDWORD) {
            auto& opval = reg.sdword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_QWORD) {
            auto& opval = reg.qword;
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SQWORD) {
            auto& opval = reg.sqword;
            f(opval, args...);
        } else {
            trap(1);
            return;
        }
    }

    template <typename F, typename... Args>
    inline void insopderef_internal(u64& addr, anonop& op, F& f, Args&... args) {
        using namespace nnasm::op;
        if (op.valtype == ASM_TYPE_BYTE) {
            using optype = u8;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SBYTE) {
            using optype = i8;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_WORD) {
            using optype = u16;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SWORD) {
            using optype = i16;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_DWORD) {
            using optype = u32;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SDWORD) {
            using optype = i32;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_QWORD) {
            using optype = u64;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SQWORD) {
            using optype = i64;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_FLOAT) {
            using optype = float;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_DOUBLE) {
            using optype = double;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        }
    }
    
    template <typename F, typename... Args>
    inline void insopderef_internal_int(u64& addr, anonop& op, F& f, Args&... args) {
        using namespace nnasm::op;
        if (op.valtype == ASM_TYPE_BYTE) {
            using optype = u8;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SBYTE) {
            using optype = i8;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_WORD) {
            using optype = u16;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SWORD) {
            using optype = i16;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_DWORD) {
            using optype = u32;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SDWORD) {
            using optype = i32;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_QWORD) {
            using optype = u64;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else if (op.valtype == ASM_TYPE_SQWORD) {
            using optype = i64;
            optype& opval = *reinterpret_cast<optype*>(memory + addr);
            f(opval, args...);
        } else {
            trap(1);
            return;
        }
    }

    template <typename F, typename... Args>
    inline void insopderef(vmregister& reg, anonop& op, F& f, Args&... args) {
        using namespace nnasm::op;
        u64 addr;
        if (op.deref & ASM_TYPE_FLOATING) {
            trap(1);
            return;
        } else if (op.deref & ASM_TYPE_SIGNED) {
            addr = absolute_address(reg.sqword);
        } else {
            addr = absolute_address(reg.qword);
        }
        if (addr > allocated) {
            trap(0);
            return;
        }
        insopderef_internal(addr, op, f, args...);
    }
    
    template <typename F, typename... Args>
    inline void insopderef_int(vmregister& reg, anonop& op, F& f, Args&... args) {
        using namespace nnasm::op;
        u64 addr;
        if (op.deref & ASM_TYPE_FLOATING) {
            trap(1);
            return;
        } else if (op.deref & ASM_TYPE_SIGNED) {
            addr = absolute_address(reg.sqword);
        } else {
            addr = absolute_address(reg.qword);
        }
        if (addr > allocated) {
            trap(0);
            return;
        }
        insopderef_internal_int(addr, op, f, args...);
    }

    template <typename T, typename U, typename V>
    inline void ins1op(V& v, U& u, T& t = {}) {
        using namespace nnasm::op;
        anonop& op = loaded.code.ops[0];
        const auto ins0oplambda = [this](auto&... args) {ins0op(args...);};
        if (op.optype == OP_REG && !op.deref) {
            insopnderef(registers[op.number], op, ins0oplambda, v, u, t);
        } else if (op.optype == OP_REG && op.deref) {
            insopderef(registers[op.number], op, ins0oplambda, v, u, t);
        } else if (op.optype == OP_VAL && !op.deref) {
            vmregister val{loaded.values[0]};
            insopnderef(val, op, ins0oplambda, v, u, t);
        } else {
            vmregister val{loaded.values[0]};
            insopderef(val, op, ins0oplambda, v, u, t);
        }
    }
    
    template <typename T, typename U, typename V>
    inline void ins1op_int(V& v, U& u, T& t = {}) {
        using namespace nnasm::op;
        anonop& op = loaded.code.ops[0];
        const auto ins0oplambda = [this](auto&... args) {ins0op(args...);};
        if (op.optype == OP_REG && !op.deref) {
            insopnderef_int(registers[op.number], op, ins0oplambda, v, u, t);
        } else if (op.optype == OP_REG && op.deref) {
            insopderef_int(registers[op.number], op, ins0oplambda, v, u, t);
        } else if (op.optype == OP_VAL && !op.deref) {
            vmregister val{loaded.values[0]};
            insopnderef_int(val, op, ins0oplambda, v, u, t);
        } else {
            vmregister val{loaded.values[0]};
            insopderef_int(val, op, ins0oplambda, v, u, t);
        }
    }

    template <typename T, typename U>
    inline void ins2op(U& u, T& t) {
        using namespace nnasm::op;
        anonop& op = loaded.code.ops[1];
        const auto ins1oplambda = [this](auto&... args) {ins1op(args...);};
        if (op.optype == OP_REG && !op.deref) {
            insopnderef(registers[op.number], op, ins1oplambda, u, t);
        } else if (op.optype == OP_REG && op.deref) {
            insopderef(registers[op.number], op, ins1oplambda, u, t);
        } else if (op.optype == OP_VAL && !op.deref) {
            vmregister val{loaded.values[1]};
            insopnderef(val, op, ins1oplambda, u, t);
        } else {
            vmregister val{loaded.values[1]};
            insopderef(val, op, ins1oplambda, u, t);
        }
    }
    
    template <typename T, typename U>
    inline void ins2op_int(U& u, T& t) {
        using namespace nnasm::op;
        anonop& op = loaded.code.ops[1];
        const auto ins1oplambda = [this](auto&... args) {ins1op_int(args...);};
        if (op.optype == OP_REG && !op.deref) {
            insopnderef_int(registers[op.number], op, ins1oplambda, u, t);
        } else if (op.optype == OP_REG && op.deref) {
            insopderef_int(registers[op.number], op, ins1oplambda, u, t);
        } else if (op.optype == OP_VAL && !op.deref) {
            vmregister val{loaded.values[1]};
            insopnderef_int(val, op, ins1oplambda, u, t);
        } else {
            vmregister val{loaded.values[1]};
            insopderef_int(val, op, ins1oplambda, u, t);
        }
    }

    template <typename T>
    inline void ins3op(T& t) {
        using namespace nnasm::op;
        anonop& op = loaded.code.ops[2];
        const auto ins2oplambda = [this](auto&... args) {ins2op(args...);};
        if (op.optype == OP_REG && !op.deref) {
            insopnderef(registers[op.number], op, ins2oplambda, t);
        } else if (op.optype == OP_REG && op.deref) {
            insopderef(registers[op.number], op, ins2oplambda, t);
        } else if (op.optype == OP_VAL && !op.deref) {
            vmregister val{loaded.values[2]};
            insopnderef(val, op, ins2oplambda, t);
        } else {
            vmregister val{loaded.values[2]};
            insopderef(val, op, ins2oplambda, t);
        }
    }
    
    template <typename T>
    inline void ins3op_int(T& t) {
        using namespace nnasm::op;
        anonop& op = loaded.code.ops[2];
        const auto ins2oplambda = [this](auto&... args) {ins2op_int(args...);};
        if (op.optype == OP_REG && !op.deref) {
            insopnderef_int(registers[op.number], op, ins2oplambda, t);
        } else if (op.optype == OP_REG && op.deref) {
            insopderef_int(registers[op.number], op, ins2oplambda, t);
        } else if (op.optype == OP_VAL && !op.deref) {
            vmregister val{loaded.values[2]};
            insopnderef_int(val, op, ins2oplambda, t);
        } else {
            vmregister val{loaded.values[2]};
            insopderef_int(val, op, ins2oplambda, t);
        }
    }
        
};
