#!/bin/env python3

import re
from typing import List, Dict
from os import path
from sys import exit
from itertools import product
from glob import glob
from collections import OrderedDict

def error(text):
    print('[PYTHON3] {}'.format(text))
    exit(1)

class TypeFlag:
    U8  = 1 << 0
    U16 = 1 << 1
    U32 = 1 << 2
    U64 = 1 << 3

    S8  = 1 << 4
    S16 = 1 << 5
    S32 = 1 << 6
    S64 = 1 << 7

    F32 = 1 << 8
    F64 = 1 << 9

    VAL = 1 << 10
    REG = 1 << 11
    MEM = 1 << 12

    DOM = 1 << 13
    SUB = 1 << 14

    USERVAL = VAL | MEM
    ANY = VAL | REG | MEM

    U = U8 | U16 | U32 | U64 
    S = S8 | S16 | S32 | S64
    F = F32 | F64
    I = U | S
    A = F | I
    NU= S | F
    NS= U | F
    NF32= I | F64
    NF64= I | F32

    TypeNames = {
        'u8': U8,
        'u16': U16,
        'u32': U32,
        'u64': U64,
        's8': S8,
        's16': S16,
        's32': S32,
        's64': S64,
        'f32': F32,
        'f64': F64,
        'u': U,
        's': S,
        'f': F,
        'i': I,
        'a': A,
        'nu': NU,
        'ns': NS,
        'nf32': NF32,
        'nf64': NF64,
        'mem': U
    }

    @staticmethod
    def typename(x):
        dtype = x & TypeFlag.A
        if bin(dtype).count('1') != 1:
            return '#invalid'
        elif dtype == TypeFlag.U8:
            return 'u8'
        elif dtype == TypeFlag.U16:
            return 'u16'
        elif dtype == TypeFlag.U32:
            return 'u32'
        elif dtype == TypeFlag.U64:
            return 'u64'
        elif dtype == TypeFlag.S8:
            return 's8'
        elif dtype == TypeFlag.S16:
            return 's16'
        elif dtype == TypeFlag.S32:
            return 's32'
        elif dtype == TypeFlag.S64:
            return 's64'
        elif dtype == TypeFlag.F32:
            return 'f32'
        elif dtype == TypeFlag.F64:
            return 'f64'
        else:
            return '#invalid'

    @staticmethod
    def extend_datatype(typeflag):
        return [1 << x for x in range(0, 10) if typeflag & (1 << x)]

    @staticmethod
    def extend_optype(typeflag):
        return [1 << x for x in range(10, 13) if typeflag & (1 << x)]

    @staticmethod
    def same_size(flag, oflag):
        if flag == TypeFlag.U8 or flag == TypeFlag.S8:
            return oflag == TypeFlag.U8 or flag == TypeFlag.S8
        elif flag == TypeFlag.U16 or flag == TypeFlag.S16:
            return oflag == TypeFlag.U16 or flag == TypeFlag.S16
        elif flag == TypeFlag.U32 or flag == TypeFlag.S32 or flag == TypeFlag.F32:
            return oflag == TypeFlag.U32 or flag == TypeFlag.S32 or flag == TypeFlag.F32
        elif flag == TypeFlag.U64 or flag == TypeFlag.S64 or flag == TypeFlag.F64:
            return oflag == TypeFlag.U64 or flag == TypeFlag.S64 or flag == TypeFlag.F64

class Operator:
    def __init__(self, _type: int= 0):
        self.type = _type

    def __repr__(self):
        optype = self.type & TypeFlag.ANY
        dtype  = self.type & TypeFlag.A
        # return str(optype)
        ret = ''
        if optype == TypeFlag.ANY:
            ret += 'any '
        elif optype == TypeFlag.REG:
            ret += 'reg '
        elif optype == TypeFlag.USERVAL:
            ret += 'val/mem '
        elif optype == TypeFlag.VAL:
            ret += 'val '
        elif optype == TypeFlag.MEM:
            ret += 'mem '
        else:
            ret += '#invalid '

        if self.type & TypeFlag.SUB:
            ret += '?'
        elif dtype == TypeFlag.A:
            ret += 'a'
        elif dtype == TypeFlag.U:
            ret += 'u'
        elif dtype == TypeFlag.S:
            ret += 's'
        elif dtype == TypeFlag.F:
            ret += 'f'
        elif dtype == TypeFlag.I:
            ret += 'i'
        elif dtype == TypeFlag.NU:
            ret += 'nu'
        elif dtype == TypeFlag.NS:
            ret += 'ns'
        elif dtype == TypeFlag.NF32:
            ret += 'nf32'
        elif dtype == TypeFlag.NF64:
            ret += 'nf64'
        else:
            ret += TypeFlag.typename(dtype)

        if self.type & TypeFlag.DOM:
            ret += '!'
        return ret

class Instruction:
    def __init__(self, name:str, code:str=None, op1:Operator=None, op2:Operator=None, op3:Operator=None, comment:str=""):
        self.name: str = name
        self.code: str = code
        ops = [op1, op2, op3]
        self.ops: List[Operator] = [x for x in ops if x is not None]
        self.comment: str = comment

        self.id: int = 0
        self.base: bool = True
        self.children: List[Instruction] = []

    def __repr__(self):
        return "({}) {} {}: {}".format(self.id, self.name, self.ops, self.comment)

loc = path.abspath(path.dirname(__file__))
proj = path.dirname(path.normpath(path.join(loc, '..', '..'))) # TODO More better
instr_loc = path.join(loc, 'instructions.txt')
instructions: List[Instruction] = []
instructions_extra: List[Instruction] = []
instructions_dict: Dict[str, Instruction] = {}

internal_regex = re.compile(r'^(-.*)$', re.RegexFlag.MULTILINE)

text = open(instr_loc, 'r').read()
instruction_text = re.findall(r'(\+[\s\S]*?%}.*?)\n', text)
internal_instruction_text = internal_regex.findall(text)

total = 0

for it in instruction_text:
    comment = it[it.rfind ('#'):]
    it = it[:-len(comment)]
    toks = it.split()
    base_name = toks[1]

    inst = Instruction(base_name, comment=comment)
    inst.id = len(instructions)
    instructions.append(inst)

    if base_name not in instructions_dict:
        instructions_dict[base_name] = [inst]
    else:
        instructions_dict[base_name].append(inst)

    cstart = 0

    for x in range(2, len(toks)):
        ort: str = toks[x]
        tok: str = toks[x]
        if tok == '{%':
            cstart = x
            break
        
        if tok[0] != '<' or tok[-1] != '>':
            error('Bad formatting on {} operand {}: Bad enclosing'.format(base_name, ort))

        tok = re.sub(r'[\<\>]', '', tok)
        if ':' not in tok:
            error('Bad formatting on {} operand {}: No separator'.format(base_name, ort))
        
        op = Operator()

        [ops, opt] = tok.split(':')
        if ops == 'any':
            op.type |= TypeFlag.ANY
        elif ops == 'reg':
            op.type |= TypeFlag.REG
        elif ops == 'val':
            op.type |= TypeFlag.USERVAL
        else:
            error('Bad operator type on {} operand {}: {}'.format(base_name, ort, ops))

        if '!' in opt:
            op.type |= TypeFlag.DOM
        elif '?' in opt:
            op.type |= TypeFlag.SUB

        if op.type & TypeFlag.DOM and op.type & TypeFlag.SUB:
            error('Operator cannot both be dominant and submissive on {} operand {}'.format(base_name, ort))

        opt = re.sub(r'[\?\!]', '', opt)
        if opt not in TypeFlag.TypeNames: 
            error('{} is not a type name on {} operand {}'.format(opt, base_name, ort))
        else:
            op.type |= TypeFlag.TypeNames[opt]
        
        inst.ops.append(op)

    if toks[cstart] != '{%' or toks[-1] != '%}':
        error('Bad enclosing on {} operand code: {{{}}}'.format(base_name, ''.join(toks[cstart:])))

    inst.code = it[it.find('{%')+2:it.rfind('%}')]
    inst.code.strip()
    if inst.code == '':
        inst.code = '/* No code here */'

    combs = []
    dom = -1
    for x in range(3):
        if x >= len(inst.ops):
            break
        op = inst.ops[x]
        if op.type & TypeFlag.SUB:
            opcombs = [x | TypeFlag.SUB for x in TypeFlag.extend_optype(op.type)]
        elif op.type & TypeFlag.DOM:   
            opcombs = [x | y | TypeFlag.DOM for x, y in list(product(TypeFlag.extend_optype(op.type), TypeFlag.extend_datatype(op.type)))]
            dom = x
        else:
            opcombs = [x | y for x, y in list(product(TypeFlag.extend_optype(op.type), TypeFlag.extend_datatype(op.type)))]
        
        if len(combs):
            combs = list(product(combs, opcombs))
            if x == 1:
                combs = [(op1, op2) for ((op1,), op2) in combs]
            if x == 2:
                combs = [(op1, op2, op3) for ((op1, op2), op3) in combs]
        else:
            combs = [(op1,) for op1 in opcombs]

    if len(combs) == 0:
        combs = [()]

    for x in range(len(combs)):
        combs[x] = tuple((Operator(y) for y in combs[x]))

        if dom > -1:
            for op in combs[x]:
                if op.type & TypeFlag.SUB:
                    op.type &= ~TypeFlag.SUB
                    op.type |= combs[x][dom].type & TypeFlag.A

    total += len(combs) 

    for comb in combs:
        name = inst.name + str(len(inst.ops))
        for op in comb:
            name += '_'
            optype = op.type & TypeFlag.ANY
            if optype == TypeFlag.REG:
                name += 'R'
            elif optype == TypeFlag.MEM:
                name += 'M'
            else:
                name += 'V'
            name += TypeFlag.typename(op.type).upper()
        nins = Instruction(name, inst.code, *comb)
        nins.id = len(instructions_extra)
        inst.children.append(nins)
        instructions_extra.append(nins)

last_normal_instruction = Instruction('_LAST')
last_normal_instruction.id = len(instructions)
instructions.append(last_normal_instruction)

for it in internal_instruction_text:
    inst = Instruction(it.split()[1])
    inst.id = len(instructions)
    instructions.append(inst)

invalid_instruction = Instruction('INV')
invalid_instruction.id = 0xFF
instructions.append(invalid_instruction)
replaceables = {'#invalid': '#invalid'}

def chunks(l, n):
    for x in range(0, len(l), n):
        yield l[x:x + n]

opcodes = ''
op_names = ''
op_names_reverse = ''
opnames = list(OrderedDict.fromkeys([x.name for x in instructions]))
for x in list(chunks(opnames, 5)):
    if len(opcodes):
        opcodes += ',\n'
    opcodes += ', '.join(x)
    for n in x:
        if len(op_names):
            op_names += ',\n'
            op_names_reverse += ',\n'
        op_names += '{{"{0}", opcode::{0}}}'.format(n)
        op_names_reverse += '{{opcode::{0}, "{0}"}}'.format(n)

replaceables['op_enum'] = opcodes
replaceables['op_names'] = op_names
replaceables['op_names_reverse'] = op_names_reverse

def gen_header(instr: Instruction):
    types = ''
    values = ''

    op_num = 1
    for op in instr.ops:
        if len(values):
            values += '\n'
        type_name = 't{}'.format(op_num)
        value_name = 'op{}'.format(op_num)
        #memory_name = 'mem{}'.format(op_num)
        real_type_name = TypeFlag.typename(op.type)
        types += 'using {} = {};\n'.format(type_name, real_type_name)
        
        optype = op.type & TypeFlag.ANY
        if optype == TypeFlag.VAL:
            values += 'align<sizeof({})>();\n'.format(type_name)
            values += 'const {0} {1} = read_from_pc<{0}>();\n'.format(type_name, value_name)
        elif optype == TypeFlag.REG:
            values += '{0}& {1} = general_registers[read_from_pc<u8>()]._{2};\n'.format(type_name, value_name, real_type_name)
        elif optype == TypeFlag.MEM:
            values += 'const {0} {1} = read_from_memory<{0}>();\n'.format(type_name, value_name)
        op_num += 1
    types += '\n' if len(types) else ''
    return types + values

format_list = ''
vm_code = ''
for x in opnames:
    if x == '_LAST':
        break
    ins_list = instructions_dict[x]
    if len(format_list):
        format_list += ',\n' 
    format_list += '{{ /* {} */\n'.format(ins_list[0].name)
    first = True
    for inst in ins_list:
        for child in inst.children:
            if not first:
                format_list += ',\n'
            first = False
            format_list += '    {{instruction{{{}}}, opcode_internal::{}}}'.format(
                ', '.join([str(x.type) for x in child.ops]), child.name
            )
            vm_code += 'case opcode_internal::{}: {{\n'.format(child.name)
            vm_code += '    ' + gen_header(child).replace('\n', '\n    ')
            vm_code += child.code
            vm_code += '\n    break;\n}\n'
    format_list += '\n}'

replaceables['format_list'] = format_list
replaceables['vm_code'] = vm_code
 
opcodes = ''
op_names = ''
opnames = list(OrderedDict.fromkeys([x.name for x in instructions_extra]))
for x in list(chunks(opnames, 5)):
    if len(opcodes):
        opcodes += ',\n'
    opcodes += ', '.join(['{}'.format(y) for y in x])
    for n in x:
        if len(op_names):
            op_names += ',\n'
        op_names += '{{opcode_internal::{0}, "{0}"}}'.format(n)

replaceables['op_enum_internal'] = opcodes
replaceables['op_names_internal'] = op_names

move_regex = re.compile(r'^(?:.*)\/\/\$move (.*)$', re.RegexFlag.MULTILINE)
replace_regex = re.compile(r'^(.*)\/\/\$replace (.*)$', re.RegexFlag.MULTILINE)

def repl_fun(tabs):
    tag = tabs.group(2)
    replacement = replaceables[tag] if tag in replaceables else '#invalid'
    return tabs.group(1) + replacement.replace('\n', '\n' + tabs.group(1))

for f in glob(path.join(loc, '*.template.*')):
    text = open(f, 'r').read()
    moveto = move_regex.search(text)
    text = text[:moveto.start()] + text[moveto.end()+1:]
    text = replace_regex.sub(repl_fun, text)
    
    moveto = path.join(proj, moveto.group(1), path.basename(f).replace('.template', '.generated').replace('.cxx', '.cpp'))
    if not path.exists(moveto):
        open(moveto, 'w').write(text)
    else:
        ntext = open(moveto, 'r').read()
        if ntext != text:
            open(moveto, 'w').write(text)