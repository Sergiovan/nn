from enum import Enum
from typing import List
from grammar import *
import type
from symbol_table import StEntry

Type = type.Type
TypeID = type.TypeID

class AstType(Enum):
    NONE = 0
    SYMBOL = 1
    BYTE = 2
    WORD = 3
    DWORD = 4
    QWORD = 5
    STRING = 6
    ARRAY = 7
    STRUCT = 8
    FUNCTION = 9
    PRE_UNARY = 10
    POST_UNARY = 11
    BINARY = 12
    BLOCK = 13

class Ast:
    pass

class AstNone(Ast):
    def __init__(self):
        self.asttype = AstType.NONE
        self.type = Type(0)

class AstSymbol(Ast):
    def __init__(self, sym: StEntry, name=""):
        self.asttype = AstType.SYMBOL
        self.sym = sym
        self.name = name

    @property
    def type(self):
        return self.sym.type # TODO

class AstByte(Ast):
    def __init__(self, value, type: Type):
        self.asttype = AstType.BYTE
        self.data = value
        self.type = type

class AstWord(Ast):
    def __init__(self, value, type: Type):
        self.asttype = AstType.WORD
        self.data = value
        self.type = type

class AstDWord(Ast):
    def __init__(self, value, type: Type):
        self.asttype = AstType.DWORD
        self.data = value
        self.type = type

class AstQWord(Ast):
    def __init__(self, value, type: Type):
        self.asttype = AstType.QWORD
        self.data = value
        self.type = type

class AstString(Ast):
    def __init__(self, value: str):
        self.asttype = AstType.STRING
        self.data = value
        # len is not needed in python
        self.type = Type(TypeID.STRING) # Todo

class AstArray(Ast):
    def __init__(self, value: List[Ast], type: Type):
        self.asttype = AstType.ARRAY
        self.data = value
        self.type = type

class AstStruct(Ast): # Struct value
    def __init__(self, elements: List[Ast], type: Type):
        self.asttype = AstType.STRUCT
        self.elements = elements
        self.type = type

class AstBlock(Ast):
    def __init__(self, stmts: List[Ast], st):
        self.asttype = AstType.BLOCK
        self.stmts = stmts
        self.st = st
        self.type = Type(0) # Todo

class AstFunction(Ast): # Function value
    def __init__(self, block: AstBlock, type: Type):
        self.asttype = AstType.FUNCTION
        self.block = block
        self.type = type

class AstUnary(Ast):
    def __init__(self, op: Symbol, stmt: Ast, type: Type = None, assignable = False, post=True):
        self.asttype = AstType.POST_UNARY if post else AstType.PRE_UNARY
        self.op = op
        self.stmt = stmt
        self.type = type or Type(0)
        self.assignable = assignable

class AstBinary(Ast):
    def __init__(self, op: Symbol, left: Ast, right: Ast, type: Type = None, assignable = False):
        self.asttype = AstType.BINARY
        self.op = op
        self.left = left
        self.right = right
        self.type = type or Type(0)
        self.assignable = assignable