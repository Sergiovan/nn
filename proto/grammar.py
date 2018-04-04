from enum import Enum

class TokenType(Enum):
    KEYWORD = 0
    SYMBOL = 1
    IDENTIFIER = 2
    COMPILER_IDENTIFIER = 3
    NUMBER = 4
    STRING = 5
    CHARACTER = 6
    EOF = 7
    NONE = 8

tokentype_to_str = ["KWD", "SYM", "IDN", "$ID", "NUM", "STR", "CHR", "EOF", "---"]

class Keyword(Enum):
    # Types
    VOID = 0
    BYTE = 1
    CHAR = 8
    SHORT = 2
    INT = 3

    LONG = 4
    SIG = 11
    FLOAT = 5
    DOUBLE = 6
    BOOL = 10

    STRING = 9
    FUN = 12
    LET = 13

    STRUCT = 14
    UNION = 15
    ENUM = 16

    # Type modifiers
    CONST = 17
    VOLATILE = 18
    SIGNED = 19
    UNSIGNED = 20

    # Control
    IF = 21
    ELSE = 22
    FOR = 23
    WHILE = 24

    DO = 25
    SWITCH = 26
    RETURN = 27

    RAISE = 28
    BREAK = 29
    CONTINUE = 30

    LEAVE = 31
    GOTO = 32
    LABEL = 33
    DEFER = 34

    #Other
    TRUE = 35
    FALSE = 36
    NULL = 37
    IMPORT = 38

    USING = 39
    NAMESPACE = 40
    CASE = 41
    AS = 42

    NEW = 43
    DELETE = 44

    #Reserved
    TRY = 45
    CATCH = 46
    AND = 47
    OR = 48

    #Invalid
    KEYWORD_INVALID = -1

class Symbol(Enum):
    POINTER = 0
    MULTIPLY = 0
    ADDRESS = 0

    GREATER = 1
    THAN_RIGHT = 1

    LESS = 2
    THAN_LEFT = 2

    TERNARY_CHOICE = 3
    COLON = 3

    COMPILER = 4

    #POINTER
    UNIQUE_POINTER = 5
    WEAK_POINTER = 6
    SHARED_POINTER = 7

    NOTHING = 8

    INCREMENT = 9
    DECREMENT = 10

    ADD = 11
    SUBTRACT = 12
    #MULTIPLY
    POWER = 13

    DIVIDE = 14
    MODULO = 15

    #ADDRESS
    DEREFERENCE = 16

    NOT = 17
    AND = 18
    OR = 19
    XOR = 20

    LNOT = 21
    LAND = 22
    LOR = 23
    LXOR = 24

    ACCESS = 25
    CONCATENATE = 26
    SPREAD = 27

    TERNARY_CONDITION = 28
    #TERNARY_CHOICE

    BIT_SET = 29
    BIT_CLEAR = 30
    BIT_CHECK = 31
    BIT_TOGGLE = 32

    SHIFT_LEFT = 33
    SHIFT_RIGHT = 34
    ROTATE_LEFT = 35
    ROTATE_RIGHT = 36

    EQUALS = 37
    NOT_EQUALS = 38

    # GREATER
    GREATER_OR_EQUALS = 39

    # LESS
    LESS_OR_EQUALS = 40

    ASSIGN = 41
    ADD_ASSIGN = 42
    SUBTRACT_ASSIGN = 43
    MULTIPLY_ASSIGN = 44
    POWER_ASSIGN = 45
    DIVIDE_ASSIGN = 46
    AND_ASSIGN = 47
    OR_ASSIGN = 48
    XOR_ASSIGN = 49
    SHIFT_LEFT_ASSIGN = 50
    SHIFT_RIGHT_ASSIGN = 51
    ROTATE_LEFT_ASSIGN = 52
    ROTATE_RIGHT_ASSIGN = 53
    CONCATENATE_ASSIGN = 54
    BIT_SET_ASSIGN = 55
    BIT_CLEAR_ASSIGN = 56
    BIT_TOGGLE_ASSIGN = 57

    COMMA = 58
    TILDE = 59
    SEMICOLON = 60
    # COLON
    PAREN_LEFT = 61
    PAREN_RIGHT = 62

    BRACE_LEFT = 63
    BRACE_RIGHT = 64

    BRACKET_LEFT = 65
    BRACKET_RIGHT = 66

    # THAN_LEFT
    # THAN_RIGHT

    # NOT SYMBOLS JUST FOR AST PURPOSES * /
    KWIF = 67
    KWELSE = 68
    KWFOR = 69
    KWFORCLASSIC = 70
    KWFOREACH = 71
    KWFORLUA = 72

    KWWHILE = 73
    KWSWITCH = 74

    KWRETURN = 75
    KWRAISE = 76
    KWGOTO = 77
    KWLABEL = 78

    KWCASE = 79

    KWDEFER = 80
    KWBREAK = 81
    KWCONTINUE = 82
    KWLEAVE = 83
    KWIMPORT = 84

    KWUSING = 85
    KWNAMESPACE = 86

    FUN_CALL = 87
    FUN_RET = 88

    SYMDECL = 89

    KWNEW = 90
    KWDELETE = 91

    SYMBOL_INVALID = -1


keyword_dict = {
    "void": Keyword.VOID,
    "byte": Keyword.BYTE,
    "char": Keyword.CHAR,
    "short": Keyword.SHORT,
    "int": Keyword.INT,
    "long": Keyword.LONG,
    "sig": Keyword.SIG,
    "float": Keyword.FLOAT,
    "double": Keyword.DOUBLE,
    "bool": Keyword.BOOL,
    "struct": Keyword.STRUCT,
    "union": Keyword.UNION,
    "enum": Keyword.ENUM,
    "string": Keyword.STRING,
    "fun": Keyword.FUN,
    "let": Keyword.LET,
    "const": Keyword.CONST,
    "volatile": Keyword.VOLATILE,
    "signed": Keyword.SIGNED,
    "unsigned": Keyword.UNSIGNED,
    "if": Keyword.IF,
    "else": Keyword.ELSE,
    "for": Keyword.FOR,
    "while": Keyword.WHILE,
    "do": Keyword.DO,
    "switch": Keyword.SWITCH,
    "return": Keyword.RETURN,
    "raise": Keyword.RAISE,
    "break": Keyword.BREAK,
    "continue": Keyword.CONTINUE,
    "leave": Keyword.LEAVE,
    "goto": Keyword.GOTO,
    "label": Keyword.LABEL,
    "defer": Keyword.DEFER,
    "true": Keyword.TRUE,
    "false": Keyword.FALSE,
    "null": Keyword.NULL,
    "import": Keyword.IMPORT,
    "using": Keyword.USING,
    "namespace": Keyword.NAMESPACE,
    "case": Keyword.CASE,
    "as": Keyword.AS,
    "new": Keyword.NEW,
    "delete": Keyword.DELETE,
    "try": Keyword.TRY,
    "catch": Keyword.CATCH,
    "and": Keyword.AND,
    "or": Keyword.OR
}

symbol_dict = {
    "$": Symbol.COMPILER,
    "*": Symbol.POINTER,
    "*!": Symbol.UNIQUE_POINTER,
    "*?": Symbol.WEAK_POINTER,
    "*+": Symbol.SHARED_POINTER,
    "---": Symbol.NOTHING,
    "++": Symbol.INCREMENT,
    "--": Symbol.DECREMENT,
    "+": Symbol.ADD,
    "-": Symbol.SUBTRACT,
    "**": Symbol.POWER,
    "/": Symbol.DIVIDE,
    "%": Symbol.MODULO,
    "@": Symbol.DEREFERENCE,
    "!": Symbol.NOT,
    "&": Symbol.AND,
    "|": Symbol.OR,
    "^": Symbol.XOR,
    "!!": Symbol.LNOT,
    "&&": Symbol.LAND,
    "||": Symbol.LOR,
    "^^": Symbol.LXOR,
    ".": Symbol.ACCESS,
    "..": Symbol.CONCATENATE,
    "...": Symbol.SPREAD,
    "?": Symbol.TERNARY_CHOICE,
    ":": Symbol.TERNARY_CONDITION,
    "@|": Symbol.BIT_SET,
    "@&": Symbol.BIT_CLEAR,
    "@?": Symbol.BIT_CHECK,
    "@^": Symbol.BIT_TOGGLE,
    "<<": Symbol.SHIFT_LEFT,
    ">>": Symbol.SHIFT_RIGHT,
    "<<<": Symbol.ROTATE_LEFT,
    ">>>": Symbol.ROTATE_RIGHT,
    "==": Symbol.EQUALS,
    "!=": Symbol.NOT_EQUALS,
    ">": Symbol.GREATER,
    ">=": Symbol.GREATER_OR_EQUALS,
    "<": Symbol.LESS,
    "<=": Symbol.LESS_OR_EQUALS,
    "=": Symbol.ASSIGN,
    "+=": Symbol.ADD_ASSIGN,
    "-=": Symbol.SUBTRACT_ASSIGN,
    "*=": Symbol.MULTIPLY_ASSIGN,
    "**=": Symbol.POWER_ASSIGN,
    "/=": Symbol.DIVIDE_ASSIGN,
    "&=": Symbol.AND_ASSIGN,
    "|=": Symbol.OR_ASSIGN,
    "^=": Symbol.XOR_ASSIGN,
    "<<=": Symbol.SHIFT_LEFT_ASSIGN,
    ">>=": Symbol.SHIFT_RIGHT_ASSIGN,
    "<<<=": Symbol.ROTATE_LEFT_ASSIGN,
    ">>>=": Symbol.ROTATE_RIGHT_ASSIGN,
    "..=": Symbol.CONCATENATE_ASSIGN,
    "@|=": Symbol.BIT_SET_ASSIGN,
    "@&=": Symbol.BIT_CLEAR_ASSIGN,
    "@^=": Symbol.BIT_TOGGLE_ASSIGN,
    ",": Symbol.COMMA,
    "~": Symbol.TILDE,
    ";": Symbol.SEMICOLON,
    "(": Symbol.PAREN_LEFT,
    ")": Symbol.PAREN_RIGHT,
    "{": Symbol.BRACE_LEFT,
    "}": Symbol.BRACE_RIGHT,
    "[": Symbol.BRACKET_LEFT,
    "]": Symbol.BRACKET_RIGHT
}

assign_symbols = (Symbol.ASSIGN, Symbol.ADD_ASSIGN, Symbol.SUBTRACT_ASSIGN, Symbol.MULTIPLY_ASSIGN,
                 Symbol.POWER_ASSIGN, Symbol.DIVIDE_ASSIGN, Symbol.AND_ASSIGN, Symbol.OR_ASSIGN, Symbol.XOR_ASSIGN,
                 Symbol.SHIFT_LEFT_ASSIGN, Symbol.SHIFT_RIGHT_ASSIGN, Symbol.ROTATE_LEFT_ASSIGN,
                 Symbol.ROTATE_RIGHT_ASSIGN, Symbol.CONCATENATE_ASSIGN, Symbol.BIT_SET_ASSIGN,
                 Symbol.BIT_CLEAR_ASSIGN, Symbol.BIT_TOGGLE_ASSIGN)