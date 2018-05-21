from nnlogger import LOGGER
from tokenizer import *
from symbol_table import *
from type_table import *
from nnast import *
import traceback
from collections import OrderedDict

class ParserException(Exception):
    def __init__(self, t: Tokenizer, message: str = "", cb = -1, cf = -1, max_chars = 80):
        msg = t.r.get_pretty_pos() + ' ' + t.r.get_line() + ': ' + message
        super().__init__(msg)

class ParserError(Exception):
    def __init__(self, msg: str):
        super().__init__(msg)

class Context:
    def __init__(self, st: SymbolTable = None, expected: Type = None):
        self.st = st or SymbolTable()
        self.expected = expected or Type(0)

class ContextContextManager:
    def __init__(self, ctxs: 'ContextStack', ctx: Context):
        self.ctxs = ctxs
        self.ctx = ctx

    def __enter__(self):
        self.ctxs.push(self.ctx)

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.ctxs.pop()

class ContextStack:
    def __init__(self, ctx: Context):
        self.stack = [ctx]

    @property
    def top(self):
        return self.stack[-1]

    def push(self, ctx: Context):
        self.stack.append(ctx)

    def pop(self):
        return self.stack.pop()

    def save_and_push(self, ctx: Context = None):
        return ContextContextManager(self, ctx or self.top)

class Parser:
    def __init__(self, t: Tokenizer):
        self.t = t
        self.st = SymbolTable()
        self.tt = TypeTable()
        self.cx = ContextStack(Context(self.st))
        self.c: Token = None
        self.next()

    @property
    def cst(self):
        return self.cx.top.st

    @property
    def etype(self):
        return self.cx.top.expected

    @etype.setter
    def etype(self, val: Type):
        self.cx.top.expected = val

    def exception(self, msg: str, cb = -1, cf = -1, max_chars = 80):
        return ParserException(self.t, msg, cb, cf, max_chars)

    def next(self):
        tmp = self.c
        self.c = self.t.next()
        return tmp

    def cis(self, param: Union[TokenType, Keyword, Symbol]):
        if isinstance(param, TokenType):
            return self.c.type == param
        elif isinstance(param, Keyword):
            return self.c.to_keyword() == param
        elif isinstance(param, Symbol):
            return self.c.to_symbol() == param
        else:
            raise ParserError("Undefined parameter type for cis()")

    def cpeek(self, param: Union[TokenType, Keyword, Symbol], lookahead=0):
        tmp = self.t.peek(lookahead)
        if isinstance(param, TokenType):
            return tmp.type == param
        elif isinstance(param, Keyword):
            return tmp.to_keyword() == param
        elif isinstance(param, Symbol):
            return tmp.to_symbol() == param

    def require_error(self, param: Union[TokenType, Keyword, Symbol]):
        if isinstance(param, TokenType):
            return 'Expected token of type {} but got {} instead'.format(param.name, self.c.type.name)
        elif isinstance(param, Keyword):
            return 'Expected keyword {} but got {} instead'.format(param.name, self.c.to_keyword().name if self.c.type is TokenType.KEYWORD else 'token type {}'.format(self.c.type.name))
        elif isinstance(param, Symbol):
            return 'Expected symbol {} but got {} instead'.format(param.name, self.c.to_symbol().name if self.c.type is TokenType.KEYWORD else 'token type {}'.format(self.c.type.name))

    def crequire(self, param: Union[TokenType, Keyword, Symbol], message: str = ''):
        if not self.cis(param):
            raise self.exception(message or self.require_error(param))

    # Must already be defined
    def iden(self):
        if not self.cis(Keyword.THIS):
            self.crequire(TokenType.IDENTIFIER)
        tmp = self.next()
        sym, _ = self.cst.search(tmp.value, True)
        if sym is None:
            raise self.exception("Symbol \"{}\" is does not exist".format(tmp.value))
        return AstSymbol(sym, tmp.value) # TODO Foolproof

    def compileriden(self):
        return AstNone() # Todo

    def number(self):
        self.crequire(TokenType.NUMBER)
        return AstQWord(self.next().to_int(), Type(TypeID.LONG)) # Todo fit

    def string(self):
        self.crequire(TokenType.STRING)
        return AstString(self.next().value)

    def character(self):
        self.crequire(TokenType.CHARACTER)
        return AstDWord(self.next().value, Type(TypeID.CHAR))

    # Expected value must be array/pointer type
    def array(self):
        self.crequire(Symbol.BRACKET_LEFT) # [
        etype = Type(TypeID.LET)
        ret = AstArray([], Type(TypeID.LET)) # Might throw

        with self.cx.save_and_push(Context(self.cst, etype)):
            while self.cis(Symbol.COMMA) or self.cis(Symbol.BRACKET_LEFT):
                self.next() # [ ,
                if self.cis(Symbol.BRACKET_RIGHT): # ]
                    break
                else:
                    exp = self.expression() # Moves us to next comma
                    if etype.uid == TypeID.LET:
                        etype.uid = exp.type.uid
                    elif etype.uid != exp.type.uid: # Todo Type comparisons
                        raise self.exception("Cannot convert {} to {} for array".format(self.etype.uid, exp.type.uid))
                    ret.data.append(exp)

        self.crequire(Symbol.BRACKET_RIGHT) # ]
        self.next()

        ttype = TypePointer(etype)
        mangled = self.tt.mangle(ttype)
        if mangled in self.tt.mangled:
            ptype = Type(self.tt.mangled[self.tt.mangle(ttype)])
        else:
            ptype = Type(self.tt.add_type(mangled))
        ret.type = ptype

        return ret

    def struct_lit(self):
        self.crequire(Symbol.BRACE_LEFT)

        ret = AstStruct([], self.etype)
        with self.cx.save_and_push():
            stype: TypeStruct = self.tt[self.etype.uid]
            for x in stype.fields:
                ret.elements.append(x.value)

            named = False
            elem = 0
            self.etype.uid = TypeID.LET

            while self.cis(Symbol.COMMA) or self.cis(Symbol.BRACE_LEFT):
                self.next() # { ,
                if named or self.cpeek(Symbol.ASSIGN):
                    named = True
                    name = self.c.value
                    self.next() # Field name
                    self.crequire(Symbol.ASSIGN)
                    self.next() # =

                    if name not in stype.field_names:
                        raise self.exception("Name " + name + " is not a field in struct " + str(stype.name))
                    pos = stype.field_names.index(name)
                    self.etype = stype.fields[pos].type
                    ret.elements[pos] = self.expression() # Todo bitfields
                    self.etype = Type(TypeID.LET)
                else:
                    self.etype = stype.fields[elem].type
                    ret.elements[elem] = self.expression() # Todo bitfields
                    self.etype = Type(TypeID.LET)
                elem += 1
        self.crequire(Symbol.BRACE_RIGHT) # }
        self.next() # }

        return ret

    def literal(self):
        if self.cis(TokenType.CHARACTER):
            return self.character()
        elif self.cis(TokenType.NUMBER):
            return self.number()
        elif self.cis(TokenType.STRING):
            return self.string()
        elif self.cis(Symbol.BRACE_LEFT):
            return self.struct_lit()
        elif self.cis(Symbol.BRACKET_LEFT):
            return self.array()
        elif self.cis(Keyword.TRUE) or self.cis(Keyword.FALSE):
            kw = self.next()
            return AstByte(int(kw.to_keyword() == Keyword.TRUE), Type(TypeID.BOOL))
        else:
            raise ParserError("literal() called on non-literal")

    def program(self):
        broken = False
        ret = AstBlock([], self.cst)

        while True:
            try:
                if self.cis(Keyword.USING):
                    ret.stmts.append(self.usingstmt())
                elif self.cis(Keyword.NAMESPACE):
                    ret.stmts.append(self.namespacestmt())
                elif self.is_pre_decl():
                    ret.stmts.append(self.declstmt())
                elif self.cis(Keyword.IMPORT):
                    ret.stmts.append(self.importstmt())
                elif self.cis(TokenType.EOF):
                    break
                else:
                    broken = True
                    break
            except ParserException as ex:
                LOGGER.error('Whoops! An error was found while parsing [{}] ({}): {}'.format(self.c, self.t.r.get_pretty_pos(), ex))
                # LOGGER.error('\n', traceback.format_exc())
                raise
            except Exception as ex:
                LOGGER.error('Exception detected. While parsing [{}] ({}):'.format(self.c, self.t.r.get_pretty_pos()))
                # LOGGER.error('\n', traceback.format_exc())
                raise
        if broken:
            raise self.exception("Top level statement not using, namespace or declaration")

        return ret

    def statement(self):
        if self.cis(TokenType.COMPILER_IDENTIFIER):
            self.compileriden()

        if self.cis(Keyword.IF):
            return self.ifstmt()
        elif self.cis(Keyword.FOR):
            return self.forstmt()
        elif self.cis(Keyword.WHILE):
            return self.whilestmt()
        elif self.cis(Keyword.SWITCH):
            return self.switchstmt()
        elif self.cis(Keyword.RETURN):
            return self.returnstmt()
        elif self.cis(Keyword.RAISE):
            return self.raisestmt()
        elif self.cis(Keyword.GOTO):
            return self.gotostmt()
        elif self.cis(Keyword.LABEL):
            return self.labelstmt()
        elif self.cis(Keyword.DEFER):
            return self.deferstmt()
        elif self.cis(Keyword.BREAK):
            return self.breakstmt()
        elif self.cis(Keyword.CONTINUE):
            return self.continuestmt()
        elif self.cis(Keyword.LEAVE):
            return self.leavestmt()
        elif self.cis(Keyword.USING):
            return self.usingstmt()
        elif self.cis(Keyword.NAMESPACE):
            return self.namespacestmt()
        elif self.is_pre_decl():
            return self.declstmt()
        elif self.cis(Symbol.BRACE_LEFT):
            return self.scope()
        elif self.is_expression():
            self.t.peek_until(Symbol.SEMICOLON) # TODO Fix
            if self.t.search_lookahead(*assign_symbols) is not None:
                return self.assstmt()
            else:
                return self.expressionstmt()
        else:
            raise self.exception("Unexpected token: " + self.c.value)

    def scopestatement(self):
        if self.cis(Keyword.IF):
            return self.ifstmt()
        elif self.cis(Keyword.FOR):
            return self.forstmt()
        elif self.cis(Keyword.WHILE):
            return self.whilestmt()
        elif self.cis(Keyword.SWITCH):
            return self.switchstmt()
        elif self.cis(Keyword.RETURN):
            return self.returnstmt()
        elif self.cis(Keyword.RAISE):
            return self.raisestmt()
        elif self.cis(Keyword.GOTO):
            return self.gotostmt()
        elif self.cis(Keyword.BREAK):
            return self.breakstmt()
        elif self.cis(Keyword.CONTINUE):
            return self.continuestmt()
        elif self.cis(Keyword.LEAVE):
            return self.leavestmt()
        elif self.cis(Symbol.BRACE_LEFT):
            return self.scope()
        elif self.is_expression():
            self.t.peek_until(Symbol.SEMICOLON) # TODO Fix
            if self.t.search_lookahead(*assign_symbols):
                return self.assstmt()
            else:
                return self.expressionstmt()
        else:
            raise self.exception("Unexpected token: " + self.c.value)

    def compileropts(self):
        self.crequire(Symbol.COMPILER) # TODO
        self.next() # $
        self.crequire(Symbol.BRACKET_LEFT)
        while self.cis(Symbol.COMMA) or self.cis(Symbol.BRACKET_LEFT):
            self.next() # , [
            self.compileriden() # TODO
        return None

    def scope(self):
        self.crequire(Symbol.BRACE_LEFT)
        st = SymbolTable(self.cst)
        ret = AstBlock([], st)

        self.next() # {

        with self.cx.save_and_push(Context(st, self.etype)):
            while not self.cis(Symbol.BRACE_RIGHT):
                ret.stmts.append(self.statement())
            self.crequire(Symbol.BRACE_RIGHT)
            self.next()

        return ret

    def ifstmt(self):
        self.crequire(Keyword.IF)
        self.next() # if

        st = SymbolTable(self.cst)
        ret = AstBinary(Symbol.KWIF, AstBlock([], st), AstNone())
        etype = self.etype
        left: AstBlock = ret.left

        with self.cx.save_and_push(Context(st, Type(TypeID.LET))):
            left.stmts.append(self.fexpression())
            while self.cis(Symbol.SEMICOLON):
                left.stmts.append(self.fexpression())

            if not self.is_booleanish(left.stmts[-1].type) and len([x for x in self.expand_type(left.stmts[-1]) if x.uid == TypeID.SIG]) == 0:
                raise self.exception("Last statement of 'if' not a boolean")

            self.etype = etype

            if self.cis(Symbol.BRACE_LEFT):
                ret.right = self.ifscope()
            elif self.cis(Keyword.DO):
                self.next() # do
                ret.right = self.scopestatement()
            else:
                raise self.exception("Expected { or 'do' after if")

        return ret

    def ifscope(self):
        self.crequire(Symbol.BRACE_LEFT)
        ret = self.scope()

        if(self.cis(Keyword.ELSE)):
            self.next() # Else
            return AstBinary(Symbol.KWELSE, ret, self.scopestatement())
        else:
            return ret

    def forstmt(self):
        self.crequire(Keyword.FOR)
        self.next() # for

        etype = self.etype
        st = SymbolTable(self.cst)
        with self.cx.save_and_push(Context(st, Type(TypeID.LET))):
            ret = AstBinary(Symbol.KWFOR, self.forcond(), AstNone())

            self.etype = etype
            if self.cis(Symbol.BRACE_LEFT):
                ret.right = self.scope()
            elif self.cis(Keyword.DO):
                self.next() # do
                ret.right = self.scopestatement()
            else:
                raise self.exception("Expected { or 'do' after for")

        return ret # Todo does this scoping work?

    def forcond(self):
        ftype = -1
        ret = AstUnary(Symbol.SYMBOL_INVALID, AstBlock([], self.cst))
        forstart = AstNone()

        if self.cis(Symbol.SEMICOLON):
            ftype = 0
        elif self.is_varclass() or self.is_type() or self.cis(Keyword.LET):
            decl = self.vardeclperiod()
            if self.cis(Symbol.COLON):
                ftype = 1
                forstart = decl
            else:
                vardecl = self.vardeclass()
                if self.cis(Symbol.BRACE_LEFT) or self.cis(Keyword.DO):
                    ftype = 2
                else:
                    ftype = 0
                forstart = AstBinary(Symbol.ASSIGN, decl, vardecl)

                for stmt in forstart.left.stmts:
                    var = stmt.stmt
                    if var.name in self.cst:
                        raise self.exception("{} already in symbol table".format(var.name))
                    else:
                        self.cst.add(var.name, var.sym)

        elif self.cis(TokenType.IDENTIFIER):
            forstart = self.assignment()
            if self.cis(Symbol.BRACE_LEFT) or self.cis(Keyword.DO):
                ftype = 2
            else:
                ftype = 0
        else:
            type = 0
            forstart = self.fexpression()

        if ftype == 0: # At the first semicolon
            allforit = AstBlock([forstart], self.cst)

            self.crequire(Symbol.SEMICOLON)
            self.next() # ;

            if not self.cis(Symbol.SEMICOLON):
                allforit.stmts.append(self.mexpression())
            else:
                allforit.stmts.append(AstNone())

            self.crequire(Symbol.SEMICOLON)
            self.next()  # ; TODO Multiple statements

            if not self.cis(Symbol.BRACE_LEFT) and not self.cis(Keyword.DO):
                allforit.stmts.append(self.mexpression())
            else:
                allforit.stmts.append(AstNone())

            ret.stmt = allforit
            ret.op = Symbol.KWFORCLASSIC
        elif ftype == 1: # At the colon
            forcolon = AstBinary(Symbol.COLON, forstart, AstNone())
            self.etype = forcolon.left.type

            self.crequire(Symbol.COLON)
            self.next() # :

            forcolon.right = self.expression()

            for stmt in forcolon.left.stmts:
                var = stmt.stmt
                if var.name in self.cst:
                    raise self.exception("{} already in symbol table".format(var.name))
                else:
                    self.cst.add(var.name, var.sym)

            ret.stmt = forcolon
            ret.op = Symbol.KWFOREACH
        elif ftype == 2: # We done boys
            if forstart.asttype != AstType.BINARY or forstart.op != Symbol.ASSIGN:
                raise self.exception("Illegal LUA for format")

            if forstart.left.asttype != AstType.BLOCK or forstart.right.asttype != AstType.BLOCK: # TODO
                raise self.exception("Illegal LUA for format")

            if len(forstart.left.stmts) != 1 or (len(forstart.right.stmts) != 2 and len(forstart.right.stmts) != 3):
                raise self.exception("Illegal LUA for format")

            ret.stmt = forstart
            ret.op = Symbol.KWFORLUA
        else:
            raise ParserError("Unknown for type")

        return ret

    def whilestmt(self):
        self.crequire(Keyword.WHILE)
        self.next() # while

        st = SymbolTable(self.cst)
        etype = self.etype
        ret = AstBinary(Symbol.KWWHILE, AstBlock([], st), AstNone())

        with self.cx.save_and_push(Context(st, Type(TypeID.LET))):
            ret.left.stmts.append(self.fexpression())
            while self.cis(Symbol.SEMICOLON):
                ret.left.stmts.append(self.fexpression())

            if not self.is_booleanish(ret.left.stmts[-1].type):
                raise self.exception("Last statement in 'while' cannot be converted to boolean")

            self.etype = etype
            if self.cis(Symbol.BRACE_LEFT):
                ret.right = self.scope()
            elif self.cis(Keyword.DO):
                self.next() # do
                ret.right = self.scopestatement()
            else:
                raise self.exception("Expected { or 'do' after while")

        return ret

    def switchstmt(self):
        self.crequire(Keyword.SWITCH)
        self.next()  # while

        st = SymbolTable(self.cst)
        etype = self.etype
        ret = AstBinary(Symbol.KWSWITCH, AstBlock([], st), AstNone())

        with self.cx.save_and_push(Context(st, Type(TypeID.LET))):
            ret.left.stmts.append(self.fexpression())
            while self.cis(Symbol.SEMICOLON):
                ret.left.stmts.append(self.fexpression())

            self.aux = ret.left.stmts[-1].type
            self.etype = etype

            ret.right = self.switchscope()

            self.aux = Type(0)

            return ret

    def switchscope(self):
        self.crequire(Symbol.BRACE_LEFT)
        self.next() # {

        ret = AstBlock([], self.cst)

        while not self.cis(Symbol.BRACE_RIGHT):
            ret.stmts.append(self.casestmt())

        self.crequire(Symbol.BRACE_RIGHT)
        self.next()

        return ret

    def casestmt(self):
        ret = AstBinary(Symbol.SYMBOL_INVALID, AstNone(), AstNone())

        if self.cis(Keyword.ELSE):
            ret.op = Symbol.KWELSE
            self.next() # else
        elif self.cis(Keyword.CASE):
            ret.op = Symbol.KWCASE
            self.next() # case

            ret.left = AstBlock([], self.cst)

            exp = self.expression()
            if exp.type != self.aux:
                pass # TODO raise self.exception("Case clause not same as switch type")
            ret.left.stmts.append(exp)
            while self.cis(Symbol.COMMA):
                self.next() # ,
                exp = self.expression()
                if exp.type != self.aux:
                    pass # TODO raise self.exception("Case clause not same as switch type")
                ret.left.stmts.append(exp) # TODO Poorman's do-while
        else:
            raise self.exception("Expected 'else' or 'case' in switch context")

        st = SymbolTable(self.cst)
        with self.cx.save_and_push(Context(st, self.etype)):
            ret.right = AstUnary(Symbol.SYMBOL_INVALID, AstNone())

            if self.cis(Keyword.CONTINUE):
                ret.right.op = Symbol.KWCONTINUE
            elif self.cis(Symbol.BRACE_LEFT):
                ret.right.op = Symbol.KWCASE
                ret.right.stmt = self.scope()
            elif self.cis(Keyword.DO):
                self.next() # do

                ret.right.op = Symbol.KWCASE
                ret.right.stmt = self.scopestatement()
            else:
                raise self.exception("Expected 'continue', 'do' or scope after switch clause")

        return ret

    def returnstmt(self):
        self.crequire(Keyword.RETURN)
        self.next() # return

        ret = AstUnary(Symbol.KWRETURN, AstNone())
        if self.cis(Keyword.VOID):
            self.next() # void
        elif not self.cis(Symbol.SEMICOLON):
            ret.stmt = AstBlock([], self.cst)
            ret.stmt.stmts.append(self.expression())
            while self.cis(Symbol.COMMA):
                self.next() # ,
                ret.stmt.stmts.append(self.expression())
        self.crequire(Symbol.SEMICOLON)
        self.next() # ;

        if self.etype.uid == TypeID.LET:
            pass # First return is always fine
        elif isinstance(ret.stmt, AstNone) and self.etype.uid != TypeID.VOID:
            raise self.exception("Returning void from non-void function")
        else:
            retypes = self.tt[self.etype.uid].returns
            num = 0
            vnum = 0
            while num < len(retypes):
                if vnum < len(ret.stmt.stmts):
                    val = ret.stmt.stmts[vnum]
                    types = self.expand_type(val)
                    for type in types:
                        if retypes[num].uid == TypeID.SIG and type.uid != TypeID.SIG:
                            ret.stmt.stmts.insert(vnum, AstQWord(0, Type(TypeID.SIG)))
                            vnum += 1
                        else:
                            if not self.can_be_weak_cast(val.type, type):
                                raise self.exception("Cannot cast return type {} to {}".format(val.type.uid, type.uid))
                        num += 1
                    vnum += 1
                else:
                    ret.stmt.stmts.append(AstQWord(0, retypes[num]))
                    num += 1

        return ret

    def raisestmt(self):
        self.crequire(Keyword.RAISE)
        self.next() # raise

        ret = AstUnary(Symbol.KWRAISE, AstBlock([], self.cst))

        ret.stmt.stmts.append(self.expression())
        while self.cis(Symbol.COMMA):
            self.next() # next
            ret.stmt.stmts.append(self.expression())

        self.crequire(Symbol.SEMICOLON)
        self.next() # ;

        # TODO check returns and add automatic sig creation

        return ret

    def gotostmt(self):
        self.crequire(Keyword.GOTO)
        self.next() # goto

        ret = AstUnary(Symbol.KWGOTO, AstString(self.c.value))
        self.next()
        self.crequire(Symbol.SEMICOLON)
        self.next() # ;

        return ret

    def labelstmt(self):
        self.crequire(Keyword.LABEL)
        self.next() # label

        ret = AstUnary(Symbol.KWLABEL, AstString(self.c.value))
        self.next()
        self.crequire(Symbol.SEMICOLON)
        self.next()  # ;

        return ret

    def deferstmt(self):
        self.crequire(Keyword.DEFER)
        self.next()  # defer

        ret = AstUnary(Symbol.KWDEFER, self.expression()) # TODO Not an expression...
        self.next()
        self.crequire(Symbol.SEMICOLON)
        self.next()  # ;

        return ret

    def breakstmt(self):
        self.crequire(Keyword.BREAK)
        self.next()  # defer
        self.crequire(Symbol.SEMICOLON)
        self.next()  # ;

        return AstUnary(Symbol.KWBREAK, AstNone())

    def continuestmt(self):
        self.crequire(Keyword.CONTINUE)
        self.next()  # defer
        self.crequire(Symbol.SEMICOLON)
        self.next()  # ;

        return AstUnary(Symbol.KWCONTINUE, AstNone())

    def leavestmt(self):
        self.crequire(Keyword.LEAVE)
        self.next()  # defer
        self.crequire(Symbol.SEMICOLON)
        self.next()  # ;

        return AstUnary(Symbol.KWLEAVE, AstNone())

    def importstmt(self):
        self.crequire(Keyword.IMPORT)
        self.next() # import

        ret = AstBinary(Symbol.KWIMPORT, AstNone(), AstNone())

        if self.cis(TokenType.STRING):
            ret.left = self.string()
        elif self.cis(TokenType.IDENTIFIER):
            t = self.next() # identifier
            ret.left = AstString(t.value)
        else:
            raise self.exception("Expected string or identifier as import")

        if self.cis(Keyword.AS):
            self.next() # as
            ret.right = AstString(self.c.value)

        # TODO actually import and handle as

        self.crequire(Symbol.SEMICOLON)
        self.next()

        return ret

    def usingstmt(self):
        self.crequire(Keyword.USING)
        self.next()  # using

        path = []

        self.crequire(TokenType.IDENTIFIER)
        path.append(self.next().value)
        while self.cis(Symbol.ACCESS):
            self.next()  # .
            self.crequire(TokenType.IDENTIFIER)
            path.append(self.next().value)

        # TODO Do the using thing

        self.crequire(Symbol.SEMICOLON)
        self.next()

        return AstUnary(Symbol.KWUSING, AstNone())

    def namespacestmt(self):
        self.crequire(Keyword.NAMESPACE)

        path = []

        self.crequire(TokenType.IDENTIFIER)
        path.append(self.next().value)
        while self.cis(Symbol.ACCESS):
            self.next() # .
            self.crequire(TokenType.IDENTIFIER)
            path.append(self.next().value)

        pathlen = len(path)
        st = self.cst
        new = False
        for k, v in enumerate(path):
            q, _ = st.search(v, k == 0)
            if q is None:
                if v != pathlen - 1:
                    raise self.exception('Path element {} in {} does not exist'.format(v, '.'.join(path)))
                new = True
                break
            elif q.entrytype != StEntryType.NAMESPACE: # TODO Final namespaces, not openable
                raise self.exception('Path element {} in {} is not a namespace'.format(v, '.'.join(path)))
            st = q.st

        if new:
            st = SymbolTable(st)

        self.crequire(Symbol.BRACE_RIGHT)
        self.next()


        with self.cx.save_and_push(Context(st)):
            nscope = self.namespacescope()

        return AstBinary(Symbol.KWNAMESPACE, nscope, AstString('.'.join(path)))

    def namespacescope(self):
        self.crequire(Symbol.BRACE_RIGHT)
        self.next() # {

        broken = False
        ret = AstBlock([], self.cst)

        while True:
            try:
                if self.cis(Keyword.USING):
                    ret.stmts.append(self.usingstmt())
                elif self.cis(Keyword.NAMESPACE):
                    ret.stmts.append(self.namespacestmt())
                elif self.is_pre_decl():
                    ret.stmts.append(self.declstmt())
                elif self.cis(TokenType.BRACE_RIGHT):
                    break
                else:
                    broken = True
                    break # TODO Panic
            except ParserException as ex:
                LOGGER.error('\n', ex)
        if broken:
            raise self.exception("Namespace statement not using, namespace or declaration")

        return ret

    def varclass(self):
        flags = 0
        while self.is_varclass():
            if self.cis(Keyword.CONST):
                flags |= TypeFlag.CONST
            elif self.cis(Keyword.VOLATILE):
                flags |= TypeFlag.VOLATILE
            elif self.cis(Keyword.SIGNED):
                flags |= TypeFlag.SIGNED
            elif self.cis(Keyword.UNSIGNED):
                flags &= ~TypeFlag.SIGNED # TODO is this python even going to work?
            else:
                raise ParserError("Illegal Varclass")
            self.next() # varclass

        return AstByte(flags, Type(0))

    def parsertype(self):
        ptrs = [" ", "!", "?", "+"]

        typestr = ""

        if not self.is_type():
            raise self.exception("Expected type")

        ntype = 0
        if self.cis(TokenType.IDENTIFIER):
            ntype = self.iden().type.uid
        elif self.cis(Keyword.FUN):
            ntype = self.functype().data
        else:
            ntype = self.c.to_keyword().value
            self.next() # iden

        typestr += str(ntype)

        while self.cis(Symbol.POINTER) or self.cis(Symbol.UNIQUE_POINTER) or self.cis(Symbol.SHARED_POINTER) or self.cis(Symbol.WEAK_POINTER):
            typestr += "*"
            typestr += ptrs[self.c.to_symbol().value - Symbol.POINTER.value]
            self.next()
            if self.is_varclass():
                flags = self.varclass().data
                typestr += chr(flags)
                self.next()
            else:
                typestr += chr(0)

        empty = False
        arr = AstBlock([], self.cst)
        while self.cis(Symbol.BRACKET_LEFT):
            typestr += "* {}".format(chr(0)) # An array is simply a naked pointer
            self.next() # [
            if self.cis(Symbol.BRACKET_RIGHT):
                empty = True
                self.next() # ]
            else:
                if empty:
                    raise self.exception("Cannot define array dimensions after inferred dimensions")
                else:
                    arr.stmts.append(self.expression())
                    self.crequire(Symbol.BRACKET_RIGHT)
                    self.next()

        type = self.tt.mangled[typestr] if typestr in self.tt.mangled else None
        if type is None:
            reduced = self.tt.unmangle(typestr)
            if reduced not in self.tt.table:
                self.tt.add_type(reduced)
            type = self.tt.table.index(reduced)

        if len(arr.stmts):
            return AstBinary(Symbol.BRACKET_LEFT, AstDWord(type, Type(0)), arr)
        else:
            return AstDWord(type, Type(0))

    def propertype(self): # TODO Reverse
        flags = self.varclass()
        uid = self.parsertype()
        return AstBinary(Symbol.ACCESS, flags, uid, Type(uid.data if isinstance(uid, AstDWord) else uid.left.data, flags.data))

    def functype(self):
        self.crequire(Keyword.FUN)
        self.next()

        typestr = ""

        if self.cis(Symbol.THAN_LEFT):
            while self.cis(Symbol.COLON) or self.cis(Symbol.THAN_LEFT):
                self.next() # :
                typestr += ':'
                pt = self.propertype()
                if isinstance(pt.right, AstBinary):
                    raise self.exception("Arrays in functions can't have specified dimensions")
                typestr += chr(pt.left.data)
                typestr += str(pt.right.data)

            self.crequire(Symbol.PAREN_LEFT)

            if not self.cpeek(Symbol.PAREN_RIGHT):
                while self.cis(Symbol.COMMA) or self.cis(Symbol.PAREN_LEFT):
                    self.next() # , (
                    typestr += ','
                    pt = self.propertype()
                    if isinstance(pt.right, AstBinary):
                        raise self.exception("Arrays in functions can't have specified dimensions")
                    typestr += chr(pt.left.data)
                    typestr += str(pt.right.data)
                self.crequire(Symbol.PAREN_RIGHT)
                self.next() # )
            else:
                self.next() # (
                self.next() # )
                typestr += ','

            self.crequire(Symbol.THAN_RIGHT)
            self.next()

            type = self.tt.mangled[typestr] if typestr in self.tt.mangled else None
            if type is None:
                type = self.tt.get_pure(self.tt.unmangle(typestr))

            return AstDWord(type, Type(0))
        else:
            return AstDWord(TypeID.FUN, Type(0))

    def declstmt(self):
        if self.cis(Keyword.STRUCT):
            return self.structdecl()
        elif self.cis(Keyword.UNION):
            return self.uniondecl()
        elif self.cis(Keyword.ENUM):
            return self.enumdecl()
        else:
            self.t.peek_until_no_arrays(Symbol.BRACE_LEFT, Symbol.SEMICOLON)
            type = -1
            depth = 0
            check_next = False
            for tok in self.t.lookahead:
                if tok.type != TokenType.SYMBOL and tok.type != TokenType.IDENTIFIER:
                    check_next = False
                    continue
                elif tok.type == TokenType.SYMBOL:
                    sym = tok.to_symbol()
                    if check_next and depth == 0:
                        if sym == Symbol.PAREN_LEFT:
                            type = 1
                            break
                        elif sym == Symbol.SEMICOLON or sym == Symbol.COMMA or sym == Symbol.ASSIGN:
                            type = 0
                            break
                    else:
                        if sym == Symbol.SEMICOLON or sym == Symbol.BRACE_LEFT:
                            break
                        elif sym == Symbol.BRACKET_LEFT:
                            depth += 1
                        elif sym == Symbol.BRACKET_RIGHT:
                            depth -= 1
                    check_next = False
                else: # Must be identifier
                    check_next = True

            if type == 0: # Variable declaration
                ret = self.vardecl()
                self.crequire(Symbol.SEMICOLON)
                self.next() # ;
                return ret
            elif type == 1:
                return self.funcdecl()
            else:
                raise self.exception("Declaration is not struct, union, enum, variable or function")


    def declstructstmt(self, id):
        if self.cis(Keyword.STRUCT):
            return self.structdecl()
        elif self.cis(Keyword.UNION):
            return self.uniondecl()
        elif self.cis(Keyword.ENUM):
            return self.enumdecl()
        else:
            self.t.peek_until_no_arrays(Symbol.BRACE_LEFT, Symbol.SEMICOLON)
            type = -1
            depth = 0
            check_next = False
            for tok in self.t.lookahead:
                if tok.type != TokenType.SYMBOL and tok.type != TokenType.IDENTIFIER:
                    check_next = False
                    continue
                elif tok.type == TokenType.SYMBOL:
                    sym = tok.to_symbol()
                    if check_next and depth == 0:
                        if sym == Symbol.PAREN_LEFT:
                            type = 1
                            break
                        elif sym == Symbol.SEMICOLON or sym == Symbol.COMMA or sym == Symbol.ASSIGN:
                            type = 0
                            break
                    else:
                        if sym == Symbol.SEMICOLON or sym == Symbol.BRACE_LEFT:
                            break
                        elif sym == Symbol.BRACKET_LEFT:
                            depth += 1
                        elif sym == Symbol.BRACKET_LEFT:
                            depth -= 1
                    check_next = False
                else:  # Must be identifier
                    check_next = True

            if type == 0:  # Variable declaration
                ret = self.vardeclstruct()
                self.crequire(Symbol.SEMICOLON)
                self.next()  # ;
                return ret
            elif type == 1:
                return self.funcdecl(id)
            else:
                raise self.exception("Declaration is not struct, union, enum, variable or function")

    def vardeclperiod(self):
        decls = AstBlock([], self.cst)
        first = True
        pt = Type(0, 0)
        while first or self.cis(Symbol.COMMA):
            if self.cis(Symbol.COMMA):
                self.next() # ,

            if self.is_varclass():
                pt.flags = self.varclass().data

            if self.cis(Keyword.LET):
                pt.uid = TypeID.LET
                self.next() # let
            elif self.is_type():
                t = self.parsertype()
                if isinstance(t, AstBinary):
                    pt.uid = t.left.data
                else:
                    pt.uid = t.data
            elif first:
                raise self.exception("Type required for first declaration")

            self.crequire(TokenType.IDENTIFIER, "Variables require a unique name")
            name = self.c.value
            self.next() # iden

            var = StVariable(pt.uid, AstDWord(0, Type(0)), pt.flags, False) # Todo correct null initialization

            decl = AstUnary(Symbol.SYMDECL, AstSymbol(var, name))
            decls.stmts.append(decl)
            first = False

        return decls

    def vardecl(self):
        ret = AstBinary(Symbol.ASSIGN, self.vardeclperiod(), AstDWord(0, Type(0))) # TODO Correct null initialization
        if self.cis(Symbol.ASSIGN):
            vals = self.vardeclass()
            ret.right = vals

            for stmt in ret.left.stmts:
                var = stmt.stmt
                if var.name in self.cst:
                    raise self.exception("{} already in symbol table".format(var.name))
                else:
                    self.cst.add(var.name, var.sym)

            self.check_assignment(ret, True)

            # TODO Check everything is fine
            # TODO Assign values in the St if needed
        else:
            vals = AstBlock([], self.cst)
            for _ in ret.left.stmts:
                vals.stmts.append(AstDWord(0, Type(0)))
            ret.right = vals

            for stmt in ret.left.stmts:
                var = stmt.stmt
                if var.name in self.cst:
                    raise self.exception("{} already in symbol table".format(var.name))
                else:
                    self.cst.add(var.name, var.sym)
        return ret

    def vardeclass(self):
        self.crequire(Symbol.ASSIGN)
        self.next() # =
        vals = AstBlock([], self.cst)

        vals.stmts.append(self.aexpression())
        while self.cis(Symbol.COMMA):
            self.next() # ,
            vals.stmts.append(self.aexpression())

        return vals

    def vardeclstruct(self):
        return self.vardecl() # TODO Structs

    def funcdecl(self, id=None):
        type = TypeFunction()

        if self.cis(Keyword.LET) or self.cis(Keyword.VOID):
            type.returns.append(Type(self.c.to_keyword().value))
            self.next()
        elif self.is_type():
            pt = self.propertype().type
            type.returns.append(pt)
            while self.cis(Symbol.COLON):
                self.next()
                pt = self.propertype().type
                type.returns.append(pt)
        else:
            raise self.exception("Expected 'let', 'void' or type")

        self.crequire(TokenType.IDENTIFIER, "Functions require a unique name")
        name = self.c.value
        type.name = name

        prev = None

        if name in self.cst: # TODO Overloading and late declaration
            prev, _ = self.cst.search(name, False)
            if prev.entrytype != StEntryType.FUNCTION:
                raise self.exception("Redeclaring identifier {} as a function".format(name))

        self.next() # iden
        self.crequire(Symbol.PAREN_LEFT)

        self.next() # (

        if id is not None:
            type.params.append(Parameter(Type(id), 'this'))

        while not self.cis(Symbol.PAREN_RIGHT):
            param = self.parameter()
            type.params.append(param)
            if not self.cis(Symbol.COMMA):
                self.crequire(Symbol.PAREN_RIGHT)
            else:
                self.next() # ,

        self.next() # )

        pureid = self.tt.get_pure(type)
        type.truetype = Type(pureid)
        typeid = self.tt.add_type(type)

        overload = None

        if prev:
            for ol in prev.overloads:
                t = self.tt[ol.type.uid]
                if t.truetype == pureid:
                    if ol.defined:
                        raise self.exception("Redeclaring overload {}".format(name))
                    overload = ol
                    break

        st = SymbolTable(self.cst)
        scope = AstBlock([], st)
        defined = False

        for param in type.params:
            if param.name:
                st.add(param.name, StVariable(param.type.uid, param.value, param.type.flags, True))


        astfunc = AstFunction(scope, Type(typeid))
        if not prev and not overload:
            stfunc = StFunction([Overload(astfunc.type, astfunc, defined)], defined)  # TODO Overloading
            self.cst.add(name, stfunc)
        elif prev and not overload:
            prev.add_overload(Overload(astfunc.type, astfunc, defined))
        else:
            overload.func = astfunc

        if self.cis(Symbol.BRACE_LEFT):
            with self.cx.save_and_push(Context(st, Type(typeid))):
                scope = self.scope()
                defined = True
        elif self.cis(Symbol.SEMICOLON):
            scope = AstBlock([], st)
            self.next() # ;
        else:
            raise self.exception("Expected { or ; after function declaration")

        astfunc.block = scope

        return AstUnary(Symbol.SYMDECL, astfunc)

    def parameter(self):
        type = self.propertype()

        if isinstance(type.right, AstBinary):
            raise self.exception("Parameters may not have defined array dimensions")

        ret = Parameter(Type(type.right.data, type.left.data))

        if self.cis(TokenType.IDENTIFIER):
            ret.name = self.c.value
            self.next() # iden

            if self.cis(Symbol.ASSIGN):
                self.next() # =
                ret.value = self.aexpression() # TODO Check for ...

        return ret

    def funcval(self):
        type = TypeFunction()

        if self.cis(Keyword.LET) or self.cis(Keyword.VOID):
            type.returns.append(Type(self.c.to_keyword().value))
            self.next()
        elif self.is_type():
            pt = self.propertype().type
            type.returns.append(pt)
            while self.cis(Symbol.COLON):
                self.next()
                pt = self.propertype().type
                type.returns.append(pt)
        else:
            raise self.exception("Expected 'let', 'void' or type")

        name = ''
        if self.cis(TokenType.IDENTIFIER):
            name = self.c.value
            self.next()

            if name in self.cst:  # TODO Overloading and late declaration
                raise self.exception("Redeclaring an identifier")

        self.crequire(Symbol.PAREN_LEFT)

        self.next()  # (

        while not self.cis(Symbol.PAREN_RIGHT):
            param = self.parameter()
            type.params.append(param)
            if not self.cis(Symbol.COMMA):
                self.crequire(Symbol.PAREN_RIGHT)
            else:
                self.next()  # ,

        self.next()  # )

        pureid = self.tt.get_pure(type)
        type.truetype = Type(pureid)
        type.name = name if name else '<anonymous>'
        typeid = self.tt.add_type(type)

        st = SymbolTable(self.cst)
        astfunc = AstFunction(AstBlock([], st), Type(typeid))
        stfunc = StFunction([Overload(astfunc.type, astfunc)], True)  # TODO Overloading

        for param in type.params:
            st.add(param.name, StVariable(param.type.uid, param.value, param.type.flags, True))

        if name:
            st.add(name, stfunc)

        self.crequire(Symbol.BRACE_LEFT, "Function values require a scope after them")
        with self.cx.save_and_push(Context(st, Type(typeid))):
            scope = self.scope()

        astfunc.block = scope

        return astfunc

    def structdecl(self):
        self.crequire(Keyword.STRUCT)
        self.next() # struct

        self.crequire(TokenType.IDENTIFIER)
        name = self.c.value
        self.next()

        if name in self.cst: # TODO Undeclared structs
            raise self.exception("Redeclaring an identifier")

        defined = False

        tttype = TypeStruct(Type(0), {}, name) # Typetable type
        id = self.tt.add_type(tttype, False)

        sttype = StType(id, False) # Symboltable type
        self.cst.add(name, sttype)

        if self.cis(Symbol.BRACE_LEFT):
            self.next() # {

            st = SymbolTable(self.cst)
            with self.cx.save_and_push(Context(st, Type(id))):
                while not self.cis(Symbol.BRACE_RIGHT) and not self.cis(TokenType.EOF):
                    if self.cis(Keyword.USING):
                        using = self.usingstmt() # TODO Merge st or whatever
                    else:
                        elem = self.declstructstmt(id)
                        if elem.asttype == AstType.POST_UNARY and elem.op == Symbol.SYMDECL:
                            name = self.tt.table[elem.stmt.type.uid].name
                            tttype.decls[name], _ = self.cst.search(name) # TODO Forward-declare, struct compare before pure type
                        elif elem.asttype == AstType.BINARY and elem.op == Symbol.ASSIGN:
                            for decl in elem.left.stmts:
                                name = decl.stmt.name
                                var = decl.stmt.sym
                                tttype.field_names.append(name)
                                tttype.fields.append(StructField(Type(var.id, var.flags), var.data))
                        else:
                            raise ParserError("Invalid return from declstructstmt()")

                self.crequire(Symbol.BRACE_RIGHT)
                self.next() # }

                if self.cis(Symbol.COMPILER):
                    self.compileropts() # TODO

            defined = True

        elif not self.cis(Symbol.SEMICOLON):
            raise self.exception("Struct declaration requires a scope or a ';' to end it")

        ttptype = self.tt.get_pure(tttype)
        tttype.truetype = Type(ttptype)

        stt = StType(id, defined)
        self.cst.add(name, stt)

        return AstUnary(Symbol.SYMDECL, AstStruct([], id))

    def uniondecl(self):
        self.crequire(Keyword.UNION)
        self.next()  # union

        self.crequire(TokenType.IDENTIFIER)
        name = self.c.value
        self.next()

        if name in self.cst:  # TODO Undeclared unions
            raise self.exception("Redeclaring an identifier")

        defined = False
        type = TypeUnion(OrderedDict(), name)

        if self.cis(Symbol.BRACE_LEFT):
            self.next()  # {

            st = SymbolTable(self.cst)
            with self.cx.save_and_push(Context(st, Type(TypeID.VOID))):
                while not self.cis(Symbol.BRACE_RIGHT) and not self.cis(TokenType.EOF):
                    elem = self.declstmt()
                    if elem.asttype == AstType.POST_UNARY and elem.op == Symbol.SYMDECL:
                        name = self.tt.table[elem.stmt.type.uid].name
                        type.decls[name], _ = self.cst.search(name)  # TODO Forward-declare, struct compare before pure type
                    elif elem.asttype == AstType.BINARY and elem.op == Symbol.ASSIGN:
                        for decl in elem.left.stmts:
                            name = decl.stmt.name
                            var = decl.stmt.sym
                            type.field_names.append(name)
                            type.fields.append(Type(var.id, var.flags)) # TODO Disable assignments for variable declarations
                    else:
                        raise ParserError("Invalid return from declstructstmt()")

                self.crequire(Symbol.BRACE_RIGHT)
                self.next()  # }

                if self.cis(Symbol.COMPILER):
                    self.compileropts() # TODO

            defined = True

        elif not self.cis(Symbol.SEMICOLON):
            raise self.exception("Union declaration requires a scope or a ';' to end it")

        id = self.tt.add_type(type)

        st = StType(id, defined)
        self.cst.add(name, st)

        return AstUnary(Symbol.SYMDECL, AstStruct([], id))

    def enumdecl(self):
        self.crequire(Keyword.ENUM)
        self.next()  # enum

        self.crequire(TokenType.IDENTIFIER)
        name = self.c.value
        self.next()

        if name in self.cst:  # TODO Undeclared enums?
            raise self.exception("Redeclaring an identifier")

        defined = False
        type = TypeEnum([], name)

        if self.cis(Symbol.BRACE_LEFT):
            self.next()  # {

            val = 0
            while not self.cis(Symbol.BRACE_RIGHT) and not self.cis(TokenType.EOF):
                elem = self.c.value
                type.names.append(elem)

            self.crequire(Symbol.BRACE_RIGHT)
            self.next()  # }

            if self.cis(Symbol.COMPILER):
                self.compileropts()  # TODO

            defined = True

        elif not self.cis(Symbol.SEMICOLON):
            raise self.exception("Enum declaration requires a scope or a ';' to end it")

        id = self.tt.add_type(type)

        st = StType(id, defined)
        self.cst.add(name, st)

        return AstUnary(Symbol.SYMDECL, AstStruct([], id))

    def assstmt(self):
        ass = self.assignment()

        self.crequire(Symbol.SEMICOLON)
        self.next() # ;

        return ass

    def assignment(self, first = None):
        ret = AstBinary(Symbol.ASSIGN, AstBlock([], self.cst), AstBlock([], self.cst))

        ret.left.stmts.append(first or self.expression())
        if not ret.left.stmts[-1].assignable:
            raise self.exception("Expression is not assignable")
        while self.cis(Symbol.COMMA):
            ret.left.stmts.append(self.expression())
            if not ret.left.stmts[-1].assignable:
                raise self.exception("Expression is not assignable")

        self.require_assignment_op()
        sym = self.next() # assignment op
        ret.op = sym.to_symbol()

        ret.right.stmts.append(self.aexpression())
        while self.cis(Symbol.COMMA):
            ret.right.stmts.append(self.aexpression())

        self.check_assignment(ret)

        return ret

    def newexpression(self):
        self.crequire(Keyword.NEW)
        self.next() # new

        ret = AstUnary(Symbol.KWNEW, AstBlock([], self.cst))

        type = self.parsertype() # TODO Remove arrays in this case

        ttype = TypePointer(Type(type.data))
        mangled = self.tt.mangle(ttype)
        ptype = Type(0)
        if mangled in self.tt.mangled:
            ptype = Type(self.tt.mangled[self.tt.mangle(ttype)])
        else:
            ptype = Type(self.tt.add_type(mangled))

        ret.stmt.stmts.append(AstDWord(1, ptype))
        while self.cis(Symbol.COMMA):
            type = self.parsertype()

            ttype = TypePointer(Type(type.data))
            mangled = self.tt.mangle(ttype)
            ptype = Type(0)
            if mangled in self.tt.mangled:
                ptype = Type(self.tt.mangled[self.tt.mangle(ttype)])
            else:
                ptype = Type(self.tt.add_type(mangled))

            ret.stmt.stmts.append(AstDWord(1, ptype))

        return ret

    def deleteexpression(self): # TODO Statement?
        self.crequire(Keyword.DELETE)
        self.next() # delete

        ret = AstUnary(Symbol.KWDELETE, AstBlock([], self.cst))

        iden = self.iden() # Needs to exist
        ret.stmt.stmts.append(iden)
        while self.cis(Symbol.COMMA):
            iden = self.iden()  # Needs to exist
            ret.stmt.stmts.append(iden)

        return ret

    def expressionstmt(self):
        ret = self.expression()
        self.crequire(Symbol.SEMICOLON)
        self.next() # ;
        return ret

    def expression(self):
        if self.cis(Keyword.NEW):
            return self.newexpression()
        elif self.cis(Keyword.DELETE):
            return self.deleteexpression()
        else:
            return self.e17()

    def e17(self):
        ret = self.e16()
        retype = self.expand_type(ret)[0]
        if self.cis(Symbol.TERNARY_CHOICE):
            if not self.is_booleanish(retype):
                raise self.exception("Ternary condition is not a boolean")
            ret.assignable = True
            ret = AstUnary(Symbol.TERNARY_CHOICE, AstBlock([ret], self.etype))
            self.next() # ?
            ret.stmt.stmts.append(self.e17())
            self.crequire(Symbol.TERNARY_CONDITION)
            self.next() # :
            ret.stmt.stmts.append(self.e17())
            if self.expand_type(ret.stmt.stmts[1]) != self.expand_type(ret.stmt.stmts[2]):
                raise self.exception("Both sides of conditional must be the same type(s)")
            ret.type = ret.stmt.stmts[1].type
        return ret

    def e16(self):
        ret = self.e15()
        while self.cis(Symbol.EQUALS) or self.cis(Symbol.NOT_EQUALS):
            ret = AstBinary(self.next().to_symbol(), ret, self.e15(), Type(TypeID.BOOL)) # == !=
        return ret

    def e15(self):
        ret = self.e14()
        while self.cis(Symbol.GREATER) or self.cis(Symbol.LESS) or self.cis(Symbol.GREATER_OR_EQUALS) or self.cis(Symbol.LESS_OR_EQUALS):
            ret = AstBinary(self.next().to_symbol(), ret, self.e14(), Type(TypeID.BOOL)) # > < >= <=
        return ret

    def e14(self):
        ret = self.e13()
        while self.cis(Symbol.LXOR):
            ret = AstBinary(self.next().to_symbol(), ret, self.e13(), Type(TypeID.BOOL)) # ^^
        return ret

    def e13(self):
        ret = self.e12()
        while self.cis(Symbol.LOR):
            ret = AstBinary(self.next().to_symbol(), ret, self.e12(), Type(TypeID.BOOL))  # ||
        return ret

    def e12(self):
        ret = self.e11()
        while self.cis(Symbol.LAND):
            ret = AstBinary(self.next().to_symbol(), ret, self.e11(), Type(TypeID.BOOL))  # &&
        return ret

    def e11(self):
        ret = self.e10()
        retype = self.expand_type(ret)[0]
        while self.cis(Symbol.XOR):
            self.next() # ^
            oth = self.e10()
            othype = self.expand_type(oth)[0]
            if not self.is_integer(retype) or not self.is_integer(othype):
                raise self.exception("Cannot XOR types {} and {} together".format(retype.uid, othype.uid))
            ret = AstBinary(Symbol.XOR, ret, oth, othype)
        return ret

    def e10(self):
        ret = self.e9()
        retype = self.expand_type(ret)[0]
        while self.cis(Symbol.OR):
            self.next()  # |
            oth = self.e9()
            othype = self.expand_type(oth)[0]
            if not self.is_integer(retype) or not self.is_integer(othype):
                raise self.exception("Cannot OR types {} and {} together".format(retype.uid, othype.uid))
            ret = AstBinary(Symbol.OR, ret, oth, othype)
        return ret

    def e9(self):
        ret = self.e8()
        retype = self.expand_type(ret)[0]
        while self.cis(Symbol.AND):
            self.next()  # &
            oth = self.e8()
            othype = self.expand_type(oth)[0]
            if not self.is_integer(retype) or not self.is_integer(othype):
                raise self.exception("Cannot AND types {} and {} together".format(retype.uid, othype.uid))
            ret = AstBinary(Symbol.AND, ret, oth, othype)
        return ret

    def e8(self):
        ret = self.e7()
        retype = self.expand_type(ret)[0]
        sym = self.c.to_symbol()
        while sym in (Symbol.BIT_SET, Symbol.BIT_TOGGLE, Symbol.BIT_CLEAR, Symbol.BIT_CHECK):
            self.next() # @| @& @? @^
            oth = self.e7()
            othype = self.expand_type(oth)[0]
            if not self.is_integer(retype) or not self.is_integer(othype):
                raise self.exception("Cannot do bit operation with {} and {}".format(retype.uid, othype.uid))
            ret = AstBinary(sym, ret, oth, retype if sym is not Symbol.BIT_CHECK else Type(TypeID.BOOL))
            sym = self.c.to_symbol()
        return ret

    def e7(self):
        ret = self.e6()
        retype = self.expand_type(ret)[0]
        sym = self.c.to_symbol()
        while sym in (Symbol.SHIFT_RIGHT, Symbol.SHIFT_LEFT, Symbol.ROTATE_RIGHT, Symbol.ROTATE_RIGHT):
            self.next()  # >> << >>> <<<
            oth = self.e6()
            othype = self.expand_type(oth)[0]
            if not self.is_integer(retype) or not self.is_integer(othype):
                raise self.exception("Cannot do bit shift with {} and {}".format(retype.uid, othype.uid))
            ret = AstBinary(sym, ret, oth, retype)
            sym = self.c.to_symbol()
        return ret

    def e6(self):
        ret = self.e5()
        retype = self.expand_type(ret)[0]
        sym = self.c.to_symbol()
        while sym in (Symbol.ADD, Symbol.SUBTRACT, Symbol.CONCATENATE):
            self.next()  # + - ..
            oth = self.e5()
            othype = self.expand_type(oth)[0]
            if sym is Symbol.CONCATENATE:
                if ((retype.uid is not TypeID.STRING or othype.uid not in (TypeID.STRING, TypeID.CHAR)) and
                    not self.point_to_same(retype, othype)):
                    raise self.exception("Cannot concatenate {} and {}".format(retype.uid, othype.uid))
            else:
                if not self.is_number(retype) or not self.is_number(othype):
                    raise self.exception("Cannot add/subtract {} and {}".format(retype.uid, othype.uid))
            ret = AstBinary(sym, ret, oth, retype)
            sym = self.c.to_symbol()
        return ret

    def e5(self):
        ret = self.e4()
        retype = self.expand_type(ret)[0]
        sym = self.c.to_symbol()
        while sym in (Symbol.MULTIPLY, Symbol.DIVIDE, Symbol.MODULO):
            self.next()  # * / %
            oth = self.e4()
            othype = self.expand_type(oth)[0]
            if not self.is_number(retype) or not self.is_number(othype):
                raise self.exception("Cannot do * / % with {} and {}".format(retype.uid, othype.uid))
            ret = AstBinary(sym, ret, oth, retype)
            sym = self.c.to_symbol()
        return ret

    def e4(self):
        ret = self.e3()
        retype = self.expand_type(ret)[0]
        if self.cis(Symbol.POWER):
            ret = AstBinary(self.next().to_symbol(), ret, self.e4())
            othype = self.expand_type(ret.right)[0]
            ret.type = Type(TypeID.DOUBLE) if self.is_real(retype) or self.is_real(othype) else Type(TypeID.LONG)
            if not self.is_number(retype) or not self.is_number(othype):
                raise self.exception("Cannot ** with {} and {}".format(retype.uid, othype.uid))
        return ret

    def e3(self):
        sym = self.c.to_symbol()
        if sym in (Symbol.INCREMENT, Symbol.DECREMENT, Symbol.ADD, Symbol.SUBTRACT, Symbol.NOT, Symbol.LNOT,
                   Symbol.THAN_LEFT, Symbol.DEREFERENCE, Symbol.POINTER):
            if sym == Symbol.THAN_LEFT:
                self.next() # <
                type = self.propertype() # TODO Signed Unsigned
                self.crequire(Symbol.THAN_RIGHT)
                self.next() # >
                return AstBinary(Symbol.THAN_LEFT, type, self.e3(), type.type)
            else:
                self.next() # symbol
                oth = self.e3()
                assignable = False
                type = self.expand_type(oth)[0]
                if sym is Symbol.LNOT:
                    if not self.is_booleanish(type):
                        raise self.exception("Cannot negate {}".format(type.uid))
                    type = Type(TypeID.BOOL)
                elif sym is Symbol.DEREFERENCE:
                    ttype = self.tt[type.uid]
                    if not isinstance(ttype, TypePointer):
                        raise self.exception("Trying to dereference non-pointer type")
                    type = ttype.at
                    assignable = True
                elif sym is Symbol.POINTER:
                    ttype = TypePointer(type)
                    ptype = Type(self.tt.mangled[self.tt.mangle(ttype)]) # TODO ?????
                    assignable = True
                    type = ptype
                elif sym in (Symbol.INCREMENT, Symbol.DECREMENT, Symbol.ADD, Symbol.SUBTRACT, Symbol.NOT):
                    if not self.is_integer(type):
                        raise self.exception("Cannot apply pre-unary {} to {}".format(sym.name, type.uid))
                return AstUnary(sym, oth, type, assignable, False)
        else:
            return self.e2()

    def e2(self):
        ret = self.e1()
        if self.cis(Symbol.SPREAD):
            ret = AstUnary(Symbol.SPREAD, ret, post=False) # TODO Type of this? What?
        return ret

    def e1(self):
        ret = self.ee()
        sym = self.c.to_symbol()
        fparam = None
        while sym in (Symbol.INCREMENT, Symbol.DECREMENT, Symbol.PAREN_LEFT, Symbol.BRACKET_LEFT, Symbol.ACCESS):
            retype = self.expand_type(ret)[0]
            self.next() # sym
            if sym in (Symbol.INCREMENT, Symbol.DECREMENT):
                if not self.is_integer(retype):
                    raise self.exception("Cannot increment or decrement {}".format(retype.uid))
                ret = AstUnary(sym, ret, retype)
            elif sym is Symbol.PAREN_LEFT:
                # typ = self.tt[ret.type.uid]
                if ret.asttype == AstType.SYMBOL and ret.sym.entrytype == StEntryType.FUNCTION:
                    typs = ret.sym.types
                else:
                    typs = [retype]

                if not self.is_function(retype):
                    raise self.exception("Cannot call non-function {}".format(retype.uid))
                args = []
                if fparam:
                    args.append(fparam)
                    fparam = None
                while not self.cis(Symbol.PAREN_RIGHT) and not self.cis(TokenType.EOF):
                    args.append(self.argument())
                    if self.cis(Symbol.COMMA):
                        self.next() # ,
                self.next() # )

                typ = None
                typid = None

                for i, ttyp in enumerate(typs):
                    typid = ttyp
                    typ = self.tt[ttyp.uid]
                    try:
                        named = False
                        num = 0
                        used = []
                        for argument in args:
                            if argument.asttype == AstType.BINARY and argument.op == Symbol.ASSIGN:
                                if typ.typetype == TypeType.FUNCTIONPURE:
                                    raise self.exception("Function parameter names unknown")
                                named = True
                                param: Parameter = typ.find_param(argument.left.data)
                                if param is None:
                                    raise ParserError("Named parameter did not exist")
                                if param in used:
                                    raise self.exception("Already set parameter {}".format(argument.left.data))
                                used.append(param)
                                if not self.can_be_weak_cast(self.expand_type(argument.right)[0], param.type):
                                    raise self.exception("Named parameter {}'s type different from argument passed".format(param.name))
                            else:
                                if named:
                                    raise self.exception("Non-named parameter after named parameters")
                                types = self.expand_type(argument)
                                for type in types:
                                    try:
                                        param: Parameter = typ.params[num]
                                    except IndexError:
                                        raise self.exception("Too many parameters")
                                    used.append(param)
                                    if not self.can_be_weak_cast(type, param if isinstance(param, Type) else param.type):
                                        raise self.exception("Parameter {}'s type different from argument passed".format(param.name))
                                    num += 1
                        for param in typ.params:
                            if param not in used:
                                if param.value is None:
                                    raise self.exception("Parameter {} had no argument passed".format(param.name))
                    except ParserException:
                        if len(typs) is 1:
                            raise
                        elif i == len(typs) - 1:
                            raise self.exception("None of the overloads fit ")
                        continue

                    for param in typ.params:
                        if param not in used:
                            args.append(param.value)
                    break

                ret = AstBinary(sym, ret, AstBlock(args, self.cst), typid)
            elif sym is Symbol.BRACKET_LEFT:
                type = self.tt[retype.uid]
                if type.typetype != TypeType.POINTER:
                    raise self.exception("Cannot get array element from non-pointer")
                exp = self.expression()
                exptype = self.expand_type(exp)[0]
                if not self.is_integer(exptype):
                    raise self.exception("Cannot index with non-integers")
                self.crequire(Symbol.BRACKET_RIGHT)
                self.next()
                at = type.at
                ret = AstBinary(sym, ret, exp, at, True)
            elif sym is Symbol.ACCESS:
                while True:
                    retype = self.expand_type(ret)[0]
                    t = self.tt[retype.uid]
                    name = self.next().value
                    if t.typetype == TypeType.POINTER and name != 'length':
                        t = self.tt[t.at.uid]
                    if t.typetype == TypeType.STRUCT or t.typetype == TypeType.UNION:
                        if t.field_by_name(name):
                            ret = AstUnary(Symbol.ACCESS, AstBlock([ret, AstString(name)], self.cst), t.field_by_name(name).type, True)
                            fparam = ret
                        elif name in t.decls:
                            fparam = ret
                            ret = AstSymbol(t.decls[name], name)
                        else:
                            f, _ = self.cst.search(name, True)
                            if f and f.entrytype == StEntryType.FUNCTION:
                                fparam = ret
                                ret = AstSymbol(f, name)
                            else:
                                raise self.exception("Cannot find {}".format(name))
                    elif t.typetype == TypeType.FUNCTION:
                        if name in t.sigs:
                            ret = AstQWord(t.sigs.index(name), Type(TypeID.SIG))
                            fparam = ret
                        else:
                            f, _ = self.cst.search(name, True)
                            if f and f.entrytype == StEntryType.FUNCTION:
                                fparam = ret
                                ret = AstSymbol(f, name)
                            else:
                                raise self.exception("Cannot find {}".format(name))
                    elif isinstance(ret, AstSymbol) and ret.sym.entrytype == StEntryType.TYPE and t.typetype == TypeType.ENUM:
                        if name in t.names:
                            ret = AstQWord(t.names.index(name), retype)
                        else:
                            raise self.exception("Cannot find {}".format(name))
                    elif t.typetype == TypeType.POINTER or t.typetype == TypeType.PRIMITIVE and t.type == PrimitiveType.STRING:
                        if name == 'length':
                            ret = AstUnary(Symbol.ACCESS, AstBlock([ret, AstString(name)], self.cst), Type(TypeID.LONG), False)
                            fparam = ret
                        else:
                            f, _ = self.cst.search(name, True)
                            if f and f.entrytype == StEntryType.FUNCTION:
                                fparam = ret
                                ret = AstSymbol(f, name)
                            else:
                                raise self.exception("Cannot find {}".format(name))
                    else:
                        f, _ = self.cst.search(name, True)
                        if f and f.entrytype == StEntryType.FUNCTION:
                            fparam = ret
                            ret = AstSymbol(f, name)
                        else:
                            raise self.exception("Cannot find {}".format(name))
                    if not self.cis(Symbol.ACCESS):
                        break
                    self.next() # .

            sym = self.c.to_symbol()
        return ret

    def ee(self):
        if self.cis(Symbol.PAREN_LEFT):
            self.next() # (
            ret = AstNone()
            if self.is_type() or self.cis(Keyword.LET) or self.cis(Keyword.VOID):
                ret = self.funcval()
            else:
                ret = self.expression()
            self.crequire(Symbol.PAREN_RIGHT)
            self.next() # )
            return ret
        elif self.is_literal():
            return self.literal()
        else:
            return self.iden()

    def argument(self):
        if self.cis(TokenType.IDENTIFIER) and self.cpeek(Symbol.ASSIGN):
            iden = self.next() # TODO Fancy shit with ...
            ftype = self.tt[self.etype.uid]
            if ftype.find_param(iden.value) is None:
                raise self.exception("{} does not name a parameter".format(iden.value))
            self.next() # =
            return AstBinary(Symbol.ASSIGN, AstString(iden.value), self.aexpression())
        else:
            return self.aexpression()

    def fexpression(self):
        if self.is_type() or self.cis(Keyword.LET) or self.cis(Keyword.VOID):
            return self.vardecl()
        else:
            return self.mexpression()

    def mexpression(self):
        exp = self.expression()
        if self.cis(Symbol.COMMA) or self.cis(Symbol.ASSIGN):
            return self.assignment(exp)
        else:
            return exp

    def aexpression(self):
        if self.is_type() or self.cis(Keyword.LET) or self.cis(Keyword.VOID):
            return self.funcval()
        elif self.cis(Symbol.NOTHING):
            self.next() # ---
            return AstNone(Type(TypeID.NOTHING)) # TODO into the trash
        elif self.cis(Keyword.NULL):
            self.next() # null
            return AstNone(Type(TypeID.NULL)) # TODO Differentiate
        else:
            return self.expression()

    def is_iden(self):
        return self.cis(TokenType.IDENTIFIER)

    def is_pre_decl(self):
        return (self.is_type() or self.is_varclass() or self.cis(Keyword.LET) or self.cis(Keyword.VOID) or
                self.cis(Keyword.STRUCT) or self.cis(Keyword.UNION) or self.cis(Keyword.ENUM))

    def is_type(self):
        return ( self.cis(Keyword.BYTE) or self.cis(Keyword.CHAR) or self.cis(Keyword.SHORT) or self.cis(Keyword.INT)
                 or self.cis(Keyword.LONG) or self.cis(Keyword.SIG) or self.cis(Keyword.FLOAT) or self.cis(Keyword.DOUBLE)
                 or self.cis(Keyword.BOOL) or self.cis(Keyword.STRING) or self.cis(Keyword.FUN)
                 or (self.cis(TokenType.IDENTIFIER) and self.cst.search(self.c.value, True)[0] is not None and
                     self.cst.search(self.c.value, True)[0].entrytype == StEntryType.TYPE)) # TODO Safeguard better

    def is_varclass(self):
        return self.cis(Keyword.CONST) or self.cis(Keyword.VOLATILE)

    def is_literal(self):
        return (self.cis(TokenType.CHARACTER) or self.cis(TokenType.NUMBER) or self.cis(TokenType.STRING) or
                self.cis(Symbol.BRACKET_LEFT) or self.cis(Symbol.BRACE_LEFT) or self.cis(Keyword.TRUE) or
                self.cis(Keyword.FALSE))

    def is_expression(self):
        return (self.cis(TokenType.IDENTIFIER) or self.cis(Symbol.PAREN_LEFT) or self.cis(Symbol.INCREMENT) or
                self.cis(Symbol.DECREMENT) or self.cis(Symbol.ADD) or self.cis(Symbol.SUBTRACT) or
                self.cis(Symbol.NOT) or self.cis(Symbol.LNOT) or self.cis(Symbol.THAN_LEFT) or
                self.cis(Symbol.DEREFERENCE) or self.cis(Symbol.ADDRESS) or self.cis(Keyword.DELETE) or
                self.cis(Keyword.NEW) or self.cis(Keyword.THIS) or self.is_literal())

    def is_fexpression(self):
        return self.is_expression() or self.is_varclass() or self.is_type() or self.cis(Keyword.LET)

    def is_assignment_op(self):
        if self.c.type != TokenType.SYMBOL:
            return False
        s = self.c.to_symbol()
        return s in assign_symbols

    def require_assignment_op(self):
        if not self.is_assignment_op():
            raise self.exception("Expected assignment operator, instead got {}".format(self.c.value))

    def can_be_weak_cast(self, frm: Type, to=None):
        to = to or self.etype
        totype = self.tt[to.uid]
        frmtype = self.tt[frm.uid]

        if frmtype.typetype == TypeType.PRIMITIVE:
            if frmtype.type == PrimitiveType.NULL:
                return totype.typetype == TypeType.POINTER or self.is_function(to)
            elif frmtype.type == PrimitiveType.NOTHING:
                return True

        if totype.typetype == TypeType.PRIMITIVE:
            if self.is_integer(to) and self.is_integer(frm):
                return True
            elif self.is_real(to) and self.is_real(frm): # TODO Sizes
                return True
        if to.uid == frm.uid:
            return True
        return False

    def can_be_cast(self, frm: Type, to=None):
        to = to or self.etype
        totype = self.tt[to.uid]
        frmtype = self.tt[frm.uid]

        if frmtype.typetype == TypeType.PRIMITIVE and frmtype.type == PrimitiveType.LET:
            return True # Undefined types can be converted to anything

        elif totype.typetype == TypeType.PRIMITIVE:
            if self.is_number(to) and self.is_numberish(frm):
                return True # Numberish things can be converted to numbers
            elif totype.type == PrimitiveType.BOOL and self.is_booleanish(frm):
                return True # Booleanish things can be converted to booleans

        elif totype.typetype == TypeType.POINTER:
            if totype.ptype == PointerType.NAKED and (self.is_integer(frm) or (frmtype.typetype == TypeType.FUNCTION) or
                    (frmtype.typetype == TypeType.POINTER)):
                return True # Integers and pointers can be converted to pointers

        elif totype.typetype == TypeType.FUNCTIONPURE or totype.typetype == TypeType.PRIMITIVE and totype.type == PrimitiveType.FUN:
            if frmtype.typetype == TypeType.FUNCTION and frmtype.truetype in (to, Type(TypeID.FUN)):
                return True
            elif frmtype.typetype == TypeType.POINTER:
                return True

        elif totype.typetype == TypeType.STRUCT:
            if frmtype.typetype == TypeType.STRUCT and frmtype.truetype == totype.truetype:
                return True

        return False

    def is_integer(self, type: Type):
        t = self.tt[type.uid]
        return t.typetype == TypeType.PRIMITIVE and t.type in (PrimitiveType.BYTE, PrimitiveType.SHORT,
                                                               PrimitiveType.INT, PrimitiveType.LONG)

    def is_real(self, type: Type):
        t = self.tt[type.uid]
        return t.typetype == TypeType.PRIMITIVE and t.type in (PrimitiveType.FLOAT, PrimitiveType.DOUBLE,
                                                               PrimitiveType.LDOUBLE)

    def is_number(self, type: Type):
        t = self.tt[type.uid]
        return t.typetype == TypeType.PRIMITIVE and t.type in (PrimitiveType.BYTE, PrimitiveType.SHORT,
                                                               PrimitiveType.INT, PrimitiveType.LONG,
                                                               PrimitiveType.FLOAT, PrimitiveType.DOUBLE,
                                                               PrimitiveType.LDOUBLE)

    def is_numberish(self, type: Type):
        t = self.tt[type.uid]
        return (self.is_number(type) or
                (t.typetype == TypeType.PRIMITIVE and t.type in (PrimitiveType.CHAR, PrimitiveType.BOOL, PrimitiveType.SIG)) or
                t.typetype in (TypeType.POINTER, TypeType.FUNCTION))

    def is_booleanish(self, type: Type):
        t = self.tt[type.uid]

        if t.typetype == TypeType.PRIMITIVE:
            pt = t.type
            return (pt == PrimitiveType.BYTE or pt == PrimitiveType.SHORT or pt == PrimitiveType.INT or
                    pt == PrimitiveType.LONG or pt == PrimitiveType.FLOAT or pt == PrimitiveType.DOUBLE or
                    pt == PrimitiveType.LDOUBLE or pt == PrimitiveType.BOOL or pt == PrimitiveType.SIG or
                    pt == PrimitiveType.FUN)
        elif t.typetype == TypeType.FUNCTION or t.typetype == TypeType.FUNCTIONPURE:
            return True
        else:
            return t.typetype == TypeType.POINTER

    def is_function(self, type: Type):
        t = self.tt[type.uid]

        if t.typetype == TypeType.FUNCTION or t.typetype == TypeType.FUNCTIONPURE:
            return True
        elif t.typetype == TypeType.PRIMITIVE and t.type == PrimitiveType.FUN:
            return True
        else:
            return False

    def point_to_same(self, a: Type, b: Type):
        at = self.tt[a.uid]
        bt = self.tt[b.uid]

        if at.typetype != TypeType.POINTER or bt.typetype != TypeType.POINTER:
            return False
        else:
            return at.at == bt.at

    def expand_type(self, ast):
        t = self.tt[ast.type.uid]
        if ast.asttype == AstType.BINARY and ast.op == Symbol.PAREN_LEFT:
            if t.typetype == TypeType.FUNCTION or t.typetype == TypeType.FUNCTIONPURE:
                return t.returns
        elif ast.asttype == AstType.POST_UNARY:
            if ast.op == Symbol.TERNARY_CHOICE:
                if t.typetype == TypeType.FUNCTION or t.typetype == TypeType.FUNCTIONPURE:
                    return t.returns
                else:
                    return [ast.type]
            elif ast.op == Symbol.KWNEW:
                return [x.type for x in ast.stmt.stmts]
        return [ast.type]

    def check_assignment(self, ast, decl=False):
        vals = ast.right

        valtypes = []
        for x in range(len(vals.stmts)):
            valtypes += [(*y, x) for y in enumerate(self.expand_type(vals.stmts[x]))]

        syms: List[AstUnary] = ast.left.stmts

        if not decl and len(syms) < len(valtypes):
            raise self.exception("Not assigning enough values")

        for i, sym in enumerate(syms):
            sym = sym.stmt if decl else sym
            if i < len(valtypes):
                pos, val, elem = valtypes[i]


                if decl and sym.sym.type == TypeID.LET:
                    if val.uid == TypeID.NULL:
                        raise self.exception("Cannot initialize let variable to null")
                    elif val.uid == TypeID.NOTHING:
                        raise self.exception("Cannot initialize let variable to nothing")
                    sym.sym.type = val.uid
                    sym.sym.flag = val.flags
                elif decl and sym.sym.type == TypeID.FUN:
                    if val.uid == TypeID.NULL:
                        raise self.exception("Cannot initialize fun variable to null")
                    elif val.uid == TypeID.NOTHING:
                        raise self.exception("Cannot initialize fun variable to nothing")
                    if not self.is_function(val):
                        raise self.exception("Assigning non-function to variable declared fun")
                    sym.sym.type = val.uid
                    sym.sym.flag = val.flags
                else:
                    if not self.can_be_weak_cast(val, sym.type):
                        raise self.exception("Cannot cast {} to {}".format(val, sym.type))

                if isinstance(sym, AstSymbol) and not sym.sym.defined:
                    if decl:
                        sym.sym.data = vals.stmts[elem] if not pos else AstBinary(Symbol.THAN_LEFT, AstQWord(pos, Type(0)),
                                                                                   vals.stmts[elem])
                    sym.sym.defined = True
            else:
                vals.stmts.append(AstDWord(0, Type(0)))  # TODO Correct null initialization