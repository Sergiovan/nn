from enum import Enum
from typing import List
from grammar import *
from nnlogger import LOGGER

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
        from type import Type
        self.asttype = AstType.NONE
        self.type = Type(0)

    def __str__(self):
        return "AstNone()"

class AstSymbol(Ast):
    def __init__(self, sym, name=""):
        from symbol_table import StEntryType
        self.asttype = AstType.SYMBOL
        self.sym = sym
        self.name = name
        self.assignable = self.sym.entrytype == StEntryType.VARIABLE

    def __str__(self):
        return "AstSymbol({}, {})".format(self.sym.entrytype, self.name)

    @property
    def type(self):
        from type import Type, TypeID
        return Type(self.sym.type, self.sym.flags if hasattr(self.sym, 'flags') else 0) # TODO

class AstByte(Ast):
    def __init__(self, value, type):
        self.asttype = AstType.BYTE
        self.data = value
        self.type = type

    def __str__(self):
        return "AstByte({}, {})".format(self.data, self.type)

class AstWord(Ast):
    def __init__(self, value, type):
        self.asttype = AstType.WORD
        self.data = value
        self.type = type

    def __str__(self):
        return "AstWord({}, {})".format(self.data, self.type)

class AstDWord(Ast):
    def __init__(self, value, type):
        self.asttype = AstType.DWORD
        self.data = value
        self.type = type

    def __str__(self):
        return "AstDWord({}, {})".format(self.data, self.type)

class AstQWord(Ast):
    def __init__(self, value, type):
        self.asttype = AstType.QWORD
        self.data = value
        self.type = type

    def __str__(self):
        return "AstQWord({}, {})".format(self.data, self.type)

class AstString(Ast):
    def __init__(self, value: str):
        from type import Type, TypeID
        self.asttype = AstType.STRING
        self.data = value
        # len is not needed in python
        self.type = Type(TypeID.STRING)

    def __str__(self):
        return "AstString({}, {})".format(self.data, self.type)

class AstArray(Ast):
    def __init__(self, value: List[Ast], type):
        self.asttype = AstType.ARRAY
        self.data = value
        self.type = type

    def __str__(self):
        return "AstArray({}, {})".format(self.data, self.type)

class AstStruct(Ast): # Struct value
    def __init__(self, elements: List[Ast], type):
        self.asttype = AstType.STRUCT
        self.elements = elements
        self.type = type

    def __str__(self):
        return "AstStruct({}, {})".format(self.elements, self.type)

class AstBlock(Ast):
    def __init__(self, stmts: List[Ast], st):
        from type import Type
        self.asttype = AstType.BLOCK
        self.stmts = stmts
        self.st = st
        self.type = Type(0) # Todo

    def __str__(self):
        return "AstBlock({}, {})".format(len(self.stmts), self.type)

class AstFunction(Ast): # Function value
    def __init__(self, block: AstBlock, type):
        self.asttype = AstType.FUNCTION
        self.block = block
        self.type = type

    def __str__(self):
        return "AstFunction({})".format(self.type)

class AstUnary(Ast):
    def __init__(self, op: Symbol, stmt: Ast, type = None, assignable = False, post=True):
        from type import Type
        self.asttype = AstType.POST_UNARY if post else AstType.PRE_UNARY
        self.op = op
        self.stmt = stmt
        self.type = type or Type(0)
        self.assignable = assignable

    def __str__(self):
        return "AstUnary{}({}, {}, {})".format('Post' if self.asttype == AstType.POST_UNARY else 'pre', self.op, self.type, self.assignable)

class AstBinary(Ast):
    def __init__(self, op: Symbol, left: Ast, right: Ast, type = None, assignable = False):
        from type import Type
        self.asttype = AstType.BINARY
        self.op = op
        self.left = left
        self.right = right
        self.type = type or Type(0)
        self.assignable = assignable

    def __str__(self):
        return "AstBinary({}, {}, {})".format(self.op, self.type, self.assignable)

def print_tree(start: Ast, indentation=''):
    def convert_indent(indent):
        ret = ''
        delet = False
        for char in indent:
            if char == '─' and delet:
                ret += ' '
            elif char == '─':
                ret += '─'
            elif char == '├':
                ret += '│'
                delet = True
            elif char == '│':
                ret += '│'
            elif char == '└':
                ret += ' '
                delet = True
            elif char == ' ':
                ret += ' '
        return ret

    print('{}{}'.format(indentation, str(start)))
    indentation = convert_indent(indentation)

    if isinstance(start, AstArray):
        for k, elem in enumerate(start.data):
            print_tree(elem, indentation + ('├─' if k is not len(start.data) - 1 else '└─'))
    elif isinstance(start, AstStruct):
        for k, elem in enumerate(start.elements):
            print_tree(elem, indentation + ('├─' if k is not len(start.elements) - 1 else '└─'))
    elif isinstance(start, AstBlock):
        for k, stmt in enumerate(start.stmts):
            print_tree(stmt, indentation + ('├─' if k is not len(start.stmts) - 1 else '└─'))
    elif isinstance(start, AstFunction):
        print_tree(start.block, indentation + '└─')
    elif isinstance(start, AstUnary):
        print_tree(start.stmt, indentation + '└─')
    elif isinstance(start, AstBinary):
        print_tree(start.left, indentation + '├─')
        print_tree(start.right, indentation + '└─')