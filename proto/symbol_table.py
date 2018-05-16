from enum import Enum
from nnlogger import LOGGER
from typing import List, Dict
from type import Overload, TypeID

class StEntryType(Enum):
    TYPE = 0
    VARIABLE = 1
    FUNCTION = 2 # Special because overloads
    NAMESPACE = 3

class StEntry:
    pass

class StType(StEntry):
    def __init__(self, id, defined: bool):
        self.entrytype = StEntryType.TYPE
        self.id = id
        self.defined = defined
        self.type = id

    def __str__(self):
        return "StType({}, {})".format(self.id, self.defined)

    def __repr__(self):
        return str(self)

class StVariable(StEntry):
    def __init__(self, id, value, flags, defined: bool):
        self.entrytype = StEntryType.VARIABLE
        self.id = id
        self.data = value
        self.flags = flags
        self.defined = defined
        self.type = id

    def __str__(self):
        return "StVariable({}, {}, {}, {})".format(self.id, self.flags, self.defined, self.data)

    def __repr__(self):
        return str(self)

class StFunction(StEntry):
    def __init__(self, overloads: List[Overload], defined: bool = True):
        self.entrytype = StEntryType.FUNCTION
        self.overloads = overloads
        self.type = TypeID.FUN
        self.types = [x.type for x in overloads]
        self.defined = defined

    def add_overload(self, overload: Overload):
        self.overloads.append(overload)
        self.types.append(overload.type)

    def __str__(self):
        return "StFunction({}, {})".format(self.overloads, self.defined)

    def __repr__(self):
        return str(self)

class StNamespace(StEntry):
    def __init__(self, st: 'SymbolTable'):
        self.entrytype = StEntryType.NAMESPACE
        self.st = st
        self.type = -1 # Todo error

    def __str__(self):
        return "StNamespace()"

    def __repr__(self):
        return str(self)

class SymbolTable:
    def __init__(self, parent: 'SymbolTable' = None):
        self.parent = parent
        self.entries: Dict[str, StEntry] = {}

    def __contains__(self, item):
        return item in self.entries

    def __delitem__(self, key):
        del self.entries[key]

    def __getitem__(self, key):
        return self.entries[key]

    def __setitem__(self, key, value):
        self.add(key, value)

    def search(self, name: str, propagate = False):
        return (self.entries[name] if name in self.entries else
                    None if not propagate or not self.parent else self.parent.search(name, True))

    def add(self, name: str, entry: StEntry):
        if name in self.entries and (not hasattr(self.entries[name], 'defined') or self.entries[name].defined):
            LOGGER.error("Entry already in entries: ", name)
            # Todo die
        self.entries[name] = entry