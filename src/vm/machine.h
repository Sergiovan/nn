#pragma once

#include "common/convenience.h"

class parser;

struct vmregister {
    union {
        u64 byte : 8;
        i64 sbyte : 8;
        u64 word : 16;
        i64 sword : 16;
        u64 dword : 32;
        i64 sdword : 32;
        u64 qword{0};
        i64 sqword;
        float fl;
        double db;
    };
};

class machine {
public:
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
    vmregister& sf = registers[14];
    vmregister& sp = registers[15];
    
    u8* memory{nullptr};
    u8* code{nullptr};
    u8* heap{nullptr};
    u8* end{nullptr};
    u64 allocated{0};
    
    parser& p;
};
