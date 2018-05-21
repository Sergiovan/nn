from enum import Enum
from typing import List, Dict

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
    NULL = 14
    NOTHING = 15

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
    STRUCTPURE = 6
    FUNCTIONPURE = 7

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
    NULL = 14
    NOTHING = 15

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

    def __str__(self):
        return "Type({}, {})".format(self.uid, self.flags)

    def __repr__(self):
        return str(self)

    def mangled(self):
        return "{}{}".format(chr(self.flags), self.uid)

class StructField:
    def __init__(self, type: Type, value = None, bits = 64, is_bitfield = False):
        self.type = type
        self.value = value
        self.bits = bits
        self.is_bitfield = is_bitfield

    def __str__(self):
        return "StructField({}, {}, {})".format(self.type, self.value, self.bits if self.is_bitfield else -1)

    def __repr__(self):
        return str(self)

class Overload:
    def __init__(self, type: Type, func = None, generic = False, defined = False):
        self.type = type
        self.func = func
        self.generic = generic
        self.defined = defined

    def __str__(self):
        return "Overload({}, {})".format(self.func, self.generic)

    def __repr__(self):
        return str(self)

class Parameter:
    def __init__(self, type: Type, name: str = "", value = None, var_length = False):
        self.type = type
        self.name = name
        self.value = value
        self.var_length = var_length

    def __str__(self):
        return "Parameter({}, {}, {}, {})".format(self.name if self.name else '---', self.type, self.value, self.var_length)

    def __repr__(self):
        return str(self)

class TypeData:
    pass

class TypePrimitive(TypeData):
    def __init__(self, type: PrimitiveType = PrimitiveType.VOID):
        self.typetype = TypeType.PRIMITIVE
        self.type = type

    @property
    def size(self):
        if self.type in (PrimitiveType.INT, PrimitiveType.FLOAT, PrimitiveType.CHAR, PrimitiveType.SIG):
            return 4 # 32 bits
        elif self.type in (PrimitiveType.LONG, PrimitiveType.DOUBLE):
            return 8 # 64 bits
        elif self.type in (PrimitiveType.LDOUBLE, PrimitiveType.STRING):
            return 16 # 128 bits
        elif self.type in (PrimitiveType.BYTE, PrimitiveType.BOOL):
            return 1 # 8 bits
        elif self.type is PrimitiveType.SHORT:
            return 2 # 16 bits
        else:
            return 0 # Invalid

    def __str__(self):
        return "TypePrimitive({})".format(self.type)

    def __repr__(self):
        return str(self)

class TypePointer(TypeData):
    def __init__(self, at: Type, flags = 0, ptype: PointerType = PointerType.NAKED):
        self.typetype = TypeType.POINTER
        self.ptype = ptype
        self.flags = flags
        self.at = at

    @property
    def size(self):
        return 8 # 64 bits

    def __str__(self):
        return "TypePointer({}, {}, {})".format(str(self.ptype), self.flags, str(self.at))

    def __repr__(self):
        return str(self)

class TypeStructPure(TypeData):
    def __init__(self, fields: List[Type] = None):
        self.typetype = TypeType.STRUCTPURE
        self.fields = fields or []
        self.size = 0

    def __str__(self):
        return "TypeStructPure({})".format(self.fields)

    def __repr__(self):
        return str(self)

class TypeStruct(TypeData):
    def __init__(self, type: Type = None, fields: Dict[str, StructField] = None, name: str = ""):
        self.typetype = TypeType.STRUCT
        self.truetype = type or Type(0)
        ff = fields or {}
        self.fields = list(ff.values())
        self.field_names = list(ff.keys())
        self.decls = {}
        self.name = name
        self.size = 0

    @property
    def names(self):
        return self.field_names + list(self.decls.keys())

    def __str__(self):
        return "TypeStruct({}, {}, {}, {})".format(self.truetype, self.name,
                                               [(self.field_names[x], str(self.fields[x])) for x in range(len(self.field_names))],
                                                   self.decls)

    def __repr__(self):
        return str(self)

    def field_by_name(self, name):
        try:
            return self.fields[self.field_names.index(name)]
        except:
            return None

class TypeUnion(TypeData):
    def __init__(self, fields: Dict[str, Type] = None, name: str = ""):
        self.typetype = TypeType.UNION
        ff = fields or {}
        self.fields = list(ff.values())
        self.field_names = list(ff.keys())
        self.decls = {}
        self.name = name
        self.size = 0

    @property
    def names(self):
        return self.field_names + list(self.decls.keys())

    def __str__(self):
        return "TypeUnion({}, {}, {})".format(self.name,
                                              [(self.field_names[x], str(self.fields[x])) for x in range(len(self.field_names))],
                                              self.decls)

    def __repr__(self):
        return str(self)

    def field_by_name(self, name):
        try:
            return self.fields[self.field_names.index(name)]
        except:
            return None

class TypeEnum(TypeData):
    def __init__(self, names: List[str] = None, name: str = ""):
        self.typetype = TypeType.ENUM
        self.names = names or []
        self.name = name

    @property
    def size(self):
        return 8 # 64 bits

    def __str__(self):
        return "TypeEnum({}, {})".format(self.name, self.names)

    def __repr__(self):
        return str(self)

class TypeFunctionPure(TypeData):
    def __init__(self, returns: List[Type] = None, params: List[Type] = None):
        self.typetype = TypeType.FUNCTIONPURE
        self.returns: List[Type] = returns or []
        self.params: List[Type] = params or []

    @property
    def size(self):
        return 8 # 64 bits

    def __str__(self):
        return "TypeFunctionPure({}, {})".format(self.returns, self.params)

    def __repr__(self):
        return str(self)

class TypeFunction(TypeData):
    def __init__(self, type: Type = None, returns: List[Type] = None, params: List[Parameter] = None, name: str = ""):
        self.typetype = TypeType.FUNCTION
        self.truetype = type or Type(TypeID.FUN)
        self.returns: List[Type] = returns or []
        self.params: List[Parameter] = params or []
        self.sigs: List[str] = []
        self.name = name

    @property
    def size(self):
        return 8 # 64 bits

    def find_param(self, name):
        for param in self.params:
            if param.name == name:
                return param
        return None

    def __str__(self):
        return "TypeFunction({}, {}, {}, {})".format(self.name, str(self.truetype), self.returns, self.params)

    def __repr__(self):
        return str(self)