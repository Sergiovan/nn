from type import *
from grammar import Symbol
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
            TypePrimitive(PrimitiveType.LET),
            TypePrimitive(PrimitiveType.NULL),
            TypePrimitive(PrimitiveType.NOTHING)
        ]
        self.mangled: Dict[str, int] = {str(x): x for x in range(len(self.table))}

    def __delitem__(self, key):
        del self.table[key]

    def __getitem__(self, key):
        return self.table[key]

    def __setitem__(self, key, value):
        self.table[key] = value

    def add_type(self, type: Union[TypeData, str], mangle = True):
        if isinstance(type, TypeData):
            if isinstance(type, (TypeStruct, TypeFunction)):
                self.table.append(type)
                id = len(self.table) - 1
                self.mangled[str(id)] = len(self.table) - 1
                return id
            else:
                self.table.append(type)
                id = len(self.table) - 1
                self.mangled[str(id)] = len(self.table) - 1
                if mangle:
                    mangled = self.mangle(type)
                    if mangled and mangled not in self.mangled:
                        self.mangled[mangled] = len(self.table) - 1
                return id
        else: # Must be a string
            return self.add_type(self.unmangle(type))

    def get_pure(self, type: Union[TypeStruct, TypeFunction]):
        if isinstance(type, (TypeStruct, TypeFunction)) and type.truetype and type.truetype.uid and \
                type.truetype.uid != TypeID.FUN:
            return self.table[type.truetype.uid]
        else:
            mangled = self.mangle(type)
            if mangled not in self.mangled:
                self.add_type(mangled)
            return self.mangled[mangled]

    def get_from(self, type):
        mangled = self.mangle(type)
        if mangled not in self.mangled:
            return self.add_type(type)
        else:
            return self.mangled[mangled]

    def mangle(self, type: TypeData, internal = False):
        def mangle_if_pointer(type: Type):
            t = self.table[type.uid]
            if isinstance(t, TypePointer):
                return self.mangle(t, True)
            else:
                return type.mangled()

        ptrs = ["* ", "*!", "*?", "*+"]
        otype = type # TODO Remove when not debugging
        ret = ''
        if isinstance(type, TypePointer):
            ret = '{}{}'.format(ptrs[type.ptype.value - PointerType.NAKED.value], chr(type.flags)) + ret
            at = self.table[type.at.uid]
            while isinstance(at, TypePointer):
                ret += '{}{}'.format(ptrs[type.ptype.value - PointerType.NAKED.value], chr(type.flags)) + ret
                type = at
                at = self.table[type.at.uid]
            ret = type.at.mangled() if internal else str(type.at.uid) + ret
        elif isinstance(type, TypeFunction) or isinstance(type, TypeFunctionPure):
            ret = ':' + ''.join('{}:'.format(mangle_if_pointer(x)) for x in type.returns)[:-1]
            ret += ',' + ''.join('{},'.format(mangle_if_pointer(x.type if isinstance(type, TypeFunction) else x)) for x in type.params)[:-1]
        elif isinstance(type, TypeStruct) or isinstance(type, TypeStructPure):
            ret = '{' + ''.join('{}{{'.format(mangle_if_pointer(x.type if isinstance(type, TypeStruct) else x)) for x in type.fields)[:-1]
        elif isinstance(type, TypePrimitive):
            ret = str(type.type.value)

        return ret

    def unmangle(self, type: str):
        def unmangle_type(type, pos, first_flags = True):
            flags = 0
            opos = pos
            if first_flags:
                flags = ord(type[pos])
                pos += 1
                opos += 1
            num = ''
            while len(type) > pos and type[pos].isdigit():
                num += type[pos]
                pos += 1
            uid = int(num)
            typ = Type(uid, flags)
            ret = self.table[uid]
            while len(type) > pos and (type[pos] == '*' or type[pos] == '['):
                if type[pos] == '[':
                    ptype = PointerType.NAKED
                    flags = 0
                    pos += 1
                    uptonow = type[opos:pos]
                else:
                    pos += 1
                    ptype = {' ': PointerType.NAKED, '!': PointerType.UNIQUE, '?': PointerType.WEAK, '+': PointerType.SHARED}[type[pos]]
                    pos += 1
                    flags = ord(type[pos])
                    pos += 1
                    uptonow = type[opos:pos]

                ret = TypePointer(typ, flags, ptype)
                if uptonow not in self.mangled:
                    self.table.append(ret)
                    self.mangled[uptonow] = len(self.table) - 1
                    typ = Type(len(self.table) - 1, flags)
                else:
                    typ = Type(self.mangled[uptonow], flags)
            return ret, pos, typ

        pos = 0
        if type[0] == ':': # Function
            fun = TypeFunctionPure()
            while type[pos] == ':':
                pos += 1
                unmangled, npos, typ = unmangle_type(type, pos)
                fun.returns.append(typ)
                pos = npos
            if type[pos] == ',' and pos != len(type) - 1:
                while pos < len(type) and type[pos] == ',':
                    pos += 1
                    unmangled, npos, typ  = unmangle_type(type, pos)
                    fun.params.append(typ)
                    pos = npos
            return fun
        elif type[0] == '{': # Struct
            struct = TypeStructPure()
            while pos < len(type) and type[pos] == '{':
                pos += 1
                unmangled, npos, typ  = unmangle_type(type, pos)
                struct.fields.append(typ) # TODO Bitfields when mangling and unmangling
                pos = npos
            return struct
        else: # Normal type
            return unmangle_type(type, 0, False)[0]

    def print(self):
        for k, v in enumerate(self.table):
            print("{}: {}".format(k, v))
        print('\n')

        for k in self.mangled:
            print("'{}': {}".format(k.replace(chr(0), '\\0'), self.mangled[k]))
