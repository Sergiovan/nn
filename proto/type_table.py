from type import *
from typing import Dict, Union

class TypeTable:
    def __init__(self):
        self.table: List[TypeData] = [
            TypePrimitive(PrimitiveType.VOID),
            TypePrimitive(PrimitiveType.BYTE),
            TypePrimitive(PrimitiveType.SHORT),
            TypePrimitive(PrimitiveType.INT),
            TypePrimitive(PrimitiveType.LONG),
            TypePrimitive(PrimitiveType.FLOAT),
            TypePrimitive(PrimitiveType.DOUBLE),
            TypePrimitive(PrimitiveType.LDOUBLE),
            TypePrimitive(PrimitiveType.CHAR),
            TypePrimitive(PrimitiveType.STRING),
            TypePrimitive(PrimitiveType.BOOL),
            TypePrimitive(PrimitiveType.SIG),
            TypePrimitive(PrimitiveType.FUN),
            TypePrimitive(PrimitiveType.LET)
        ]
        self.mangled: Dict[str, int] = {str(x): x for x in range(len(self.table))}

    def __delitem__(self, key):
        del self.table[key]

    def __getitem__(self, key):
        return self.table[key]

    def __setitem__(self, key, value):
        self.table[key] = value

    def add_type(self, type: Union[TypeData, str]):
        if isinstance(type, TypeData):
            self.table.append(type)
            id = len(self.table) - 1
            self.mangled[str(id)] = len(self.table) - 1 # Properly mangle
        else: # Must be a string
            # Mangling and stuff
            pass

    def mangle(self, type: TypeData):
        pass