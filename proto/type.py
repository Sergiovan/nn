from enum import Enum
import _ast
from typing import List
from collections import OrderedDict

Ast = _ast.Ast

class TypeID:
    VOID = 0
    BYTE = 1
    SHORT = 2
    INT = 3
    LONG = 4
    FLOAT = 5
    DOUBLE = 6
    LDOUBLE = 7
    CHAR = 8
    STRING = 9
    BOOL = 10
    SIG = 11
    FUN = 12
    LET = 13

class TypeFlag:
    VOLATILE = 1 << 0
    CONST = 1 << 1
    SIGNED = 1 << 2

class TypeType(Enum):
    PRIMITIVE = 0
    POINTER = 1
    STRUCT = 2
    UNION = 3
    ENUM = 4
    FUNCTION = 5

class PrimitiveType(Enum):
    VOID = 0
    BYTE = 1
    SHORT = 2
    INT = 3
    LONG = 4
    FLOAT = 5
    DOUBLE = 6
    LDOUBLE = 7
    CHAR = 8
    STRING = 9
    BOOL = 10
    SIG = 11
    FUN = 12
    LET = 13

class PointerType(Enum):
    NAKED = 0
    SHARED = 1
    WEAK = 2
    UNIQUE = 3

class Type:
    def __init__(self, uid, flags = 0):
        self.uid = uid
        self.flags = flags

    def __cmp__(self, other):
        return isinstance(other, Type) and self.uid == other.uid

class StructField:
    def __init__(self, type: Type, value: Ast = None, bits = 64, is_bitfield = False):
        self.type = type
        self.value = value
        self.bits = bits
        self.is_bitfield = is_bitfield

class Parameter:
    def __init__(self, type: Type, name: str = "", value: Ast = None, var_length = False):
        self.type = type
        self.name = name
        self.value = value
        self.var_length = var_length

class TypeData:
    pass

class TypePrimitive(TypeData):
    def __init__(self, type: PrimitiveType = PrimitiveType.VOID):
        self.typetype = TypeType.PRIMITIVE
        self.type = type

class TypePointer(TypeData):
    def __init__(self, at: Type, type: PointerType = PointerType.NAKED):
        self.typetype = TypeType.POINTER
        self.type = type
        self.at = at

class TypeStruct(TypeData):
    def __init__(self, fields: OrderedDict[str, StructField], name: str = ""):
        self.typetype = TypeType.STRUCT
        self.fields = list(fields.values())
        self.field_names = list(fields.keys())
        self.name = name

    def field_by_name(self, name):
        return self.fields[self.field_names.index(name)]

class TypeUnion(TypeData):
    def __init__(self, fields: OrderedDict[str, Type], name: str = ""):
        self.typetype = TypeType.UNION
        self.fields = list(fields.values())
        self.field_names = list(fields.keys())
        self.name = name

class TypeEnum(TypeData):
    def __init__(self, names: List[str], name: str = ""):
        self.typetype = TypeType.ENUM
        self.names = names
        self.name = name

class TypeFunction(TypeData):
    def __init__(self, returns: List[Type] = None, params: List[Parameter] = None):
        self.typetype = TypeType.FUNCTION
        self.returns: List[Type] = returns or []
        self.params: List[Parameter] = params or []