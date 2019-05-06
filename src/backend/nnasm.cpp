#include "backend/nnasm.h"

using namespace nnasm;

// format::operand::operand(u16 raw) : raw(raw) { }

format::instruction::instruction(u16 op1, u16 op2, u16 op3) { 
    instruction::op[0].raw = op1;
    instruction::op[1].raw = op2;
    instruction::op[2].raw = op3;
    _empty = 0;
}

const std::map<nnasm::opcode, std::vector<format::instruction>> format::get_formats() {
    using namespace format;
    
    std::map<opcode, std::vector<instruction>> ret{};
    
    auto insert_0 = [&ret] (opcode code) {
        ret.insert({code, {{}}});
    };
    
    auto insert = [&ret] (opcode code, instruction instr) {
        auto ptr = ret.find(code);
        if (ptr == ret.end()) {
            ret.insert({code, {instr}});
        } else {
            ptr->second.push_back(instr);
        }
    };
    
    auto insert_cmp_sfd = [&ret] (opcode code) {
        const std::string scode = op_to_name.at(code);
        
        ret.insert({code, {{raw::any_uint, raw::any_uint}}});
        ret.insert({name_to_op.at('S' + scode), {{raw::any_sint, raw::any_sint}}});
        ret.insert({name_to_op.at('F' + scode), {{raw::any_float, raw::any_float}}});
        ret.insert({name_to_op.at('D' + scode), {{raw::any_double, raw::any_double}}});
    };
    
    auto insert_cast = [&ret] (opcode code, u16 fromtype, u16 totype) {
        ret.insert({code, {
            {(u16) (raw::mem | raw::reg | fromtype)}, 
            {(u16) (raw::any_target | fromtype), (u16) (raw::mem | raw::reg | totype)}
        }});
    };
    
    auto insert_s_arithmetic = [&ret] (opcode code, bool single_op = false) {
        const std::string scode = op_to_name.at(code);
        
        if (single_op) {
            ret.insert({code, {{raw::any_uint, raw::reg | raw::uint}, {raw::any_uint, raw::any_uint, raw::reg | raw::uint}}});
            ret.insert({name_to_op.at('S' + scode), {{raw::any_sint, raw::reg | raw::sint}, {raw::any_sint, raw::any_sint, raw::reg | raw::sint}}});
        } else {
            ret.insert({code, {{raw::reg | raw::uint}, {raw::any_uint, raw::reg | raw::uint}}});
            ret.insert({name_to_op.at('S' + scode), {{raw::reg | raw::sint}, {raw::any_sint, raw::reg | raw::sint}}});
        }
    };
    
    auto insert_sfd_arithmetic = [&ret] (opcode code, bool single_op = false) {
        const std::string scode = op_to_name.at(code);
        
        if (!single_op) {
            ret.insert({code, {{raw::any_uint, raw::reg | raw::uint}, {raw::any_uint, raw::any_uint, raw::reg | raw::uint}}});
            ret.insert({name_to_op.at('S' + scode), {{raw::any_sint, raw::reg | raw::sint}, {raw::any_sint, raw::any_sint, raw::reg | raw::sint}}});
            ret.insert({name_to_op.at('F' + scode), {{raw::any_float, raw::reg | raw::_float}, {raw::any_float, raw::any_float, raw::reg | raw::_float}}});
            ret.insert({name_to_op.at('D' + scode), {{raw::any_double, raw::reg | raw::_double}, {raw::any_double, raw::any_double, raw::reg | raw::_double}}});
        } else {
            ret.insert({code, {{raw::reg | raw::uint}, {raw::any_uint, raw::reg | raw::uint}}});
            ret.insert({name_to_op.at('S' + scode), {{raw::reg | raw::sint}, {raw::any_sint, raw::reg | raw::sint}}});
            ret.insert({name_to_op.at('F' + scode), {{raw::reg | raw::_float}, {raw::any_float, raw::reg | raw::_float}}});
            ret.insert({name_to_op.at('D' + scode), {{raw::reg | raw::_double}, {raw::any_double, raw::reg | raw::_double}}});
        }
    };
    
    auto insert_sfd_no_u_arithmetic = [&ret] (opcode code) {
        const std::string scode = op_to_name.at(code);
        
        ret.insert({code, {{raw::reg | raw::sint}, {raw::any_sint, raw::reg | raw::sint}}});
        ret.insert({name_to_op.at('F' + scode.substr(1)), {{raw::reg | raw::_float}, {raw::any_float, raw::reg | raw::_float}}});
        ret.insert({name_to_op.at('D' + scode.substr(1)), {{raw::reg | raw::_double}, {raw::any_double, raw::reg | raw::_double}}});
    };
    
    auto insert_shift_arithmetic = [&ret] (opcode code) {
        const std::string scode = op_to_name.at(code);
        
        ret.insert({code, {{raw::any_byte, raw::reg | raw::uint}, {raw::any_byte, raw::any_uint, raw::reg | raw::uint}}});
        ret.insert({name_to_op.at('S' + scode), {{raw::any_byte, raw::reg | raw::sint}, {raw::any_byte, raw::any_sint, raw::reg | raw::sint}}});
    };
    
    auto insert_shift_arithmetic_nos = [&ret] (opcode code) {
        ret.insert({code, {{raw::any_byte, raw::reg | raw::uint}, {raw::any_byte, raw::any_uint, raw::reg | raw::uint}}});
    };
    
    auto insert_bit_logical = [&ret] (opcode code, bool single_op = false) {
        
        if (single_op) {
            ret.insert({code, {{raw::any, raw::any_reg}, {raw::any, raw::any, raw::any_reg}}});
        } else {
            ret.insert({code, {{raw::any_reg}, {raw::any, raw::any_reg}}});
        }
    };
    
    insert_0(opcode::NOP);
    insert(opcode::LOAD, {raw::mem_loc, raw::any_reg});
    insert(opcode::STOR, {raw::any_reg, raw::mem_loc});
    insert(opcode::MOV, {raw::any, raw::any_reg});
    insert(opcode::CPY, {raw::mem_loc, raw::mem_loc, raw::any_uint});
    insert(opcode::ZRO, {raw::mem_loc, raw::any_uint});
    insert(opcode::SET, {raw::any_byte, raw::mem_loc, raw::any_uint});
    insert_0(opcode::BRK);
    insert_0(opcode::HLT);
    
    insert(opcode::CZRO, {raw::any});
    insert(opcode::CNZR, {raw::any});
    insert(opcode::CEQ, {raw::any, raw::any});
    insert(opcode::CNEQ, {raw::any, raw::any});
    insert(opcode::CBS, {raw::any, raw::any_byte});
    insert(opcode::CBNS, {raw::any, raw::any_byte});
    
    insert_cmp_sfd(opcode::CLT);
    insert_cmp_sfd(opcode::CLE);
    insert_cmp_sfd(opcode::CGT);
    insert_cmp_sfd(opcode::CGE);
    
    insert(opcode::JMP, {raw::mem_loc});
    insert(opcode::JMPR, {raw::any_uint});
    insert(opcode::SJMPR, {raw::any_sint});
    insert(opcode::JCH, {raw::mem_loc});
    insert(opcode::JNCH, {raw::mem_loc});
    
    insert(opcode::PUSH, {raw::any});
    insert(opcode::PUSH, {raw::mem_loc, raw::any_uint});
    insert(opcode::POP, {raw::imm | raw::mem | raw::uint});
    insert(opcode::POP, {raw::any_reg});
    insert(opcode::BTIN, {raw::any_uint});
    insert(opcode::CALL, {raw::mem_loc});
    insert_0(opcode::RET);
    
    insert_cast(opcode::CSTU, raw::sint, raw::uint);
    insert_cast(opcode::CSTF, raw::sint, raw::_float);
    insert_cast(opcode::CSTD, raw::sint, raw::_double);
    insert_cast(opcode::CUTS, raw::uint, raw::sint);
    insert_cast(opcode::CUTF, raw::uint, raw::_float);
    insert_cast(opcode::CUTD, raw::uint, raw::_double);
    insert_cast(opcode::CFTS, raw::_float, raw::sint);
    insert_cast(opcode::CFTU, raw::_float, raw::uint);
    insert_cast(opcode::CFTD, raw::_float, raw::_double);
    insert_cast(opcode::CDTS, raw::_double, raw::sint);
    insert_cast(opcode::CDTU, raw::_double, raw::uint);
    insert_cast(opcode::CDTF, raw::_double, raw::_float);
    
    insert_sfd_arithmetic(opcode::ADD);
    insert_s_arithmetic(opcode::INC, true);
    insert_sfd_arithmetic(opcode::SUB);
    insert_s_arithmetic(opcode::DEC, true);
    insert_sfd_arithmetic(opcode::MUL);
    insert_sfd_arithmetic(opcode::DIV);
    insert_s_arithmetic(opcode::MOD);
    insert_sfd_no_u_arithmetic(opcode::SABS);
    insert_sfd_no_u_arithmetic(opcode::SNEG);
    insert_shift_arithmetic(opcode::SHR);
    insert_shift_arithmetic(opcode::SHL);
    insert_shift_arithmetic_nos(opcode::RTR);
    insert_shift_arithmetic_nos(opcode::RTL);
    insert_bit_logical(opcode::AND);
    insert_bit_logical(opcode::OR);
    insert_bit_logical(opcode::XOR);
    insert_bit_logical(opcode::NOT);
    
    return ret;
}


std::ostream& operator<<(std::ostream& os, opcode code) {
    return os << op_to_name.at(code);
}
