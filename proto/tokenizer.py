from reader import Reader
from collections import deque
from grammar import *
import re
from nnlogger import LOGGER
from typing import Deque

class Token:
    def __init__(self, value, type=TokenType.NONE):
        self.value = value
        self.type = type

    def to_int(self):
        # Todo parse cool integer literals
        return int(self.value)

    def to_double(self):
        return float(self.value)

    def to_char(self):
        # Todo UTF-8
        return self.value

    def to_string(self):
        return self.value

    def to_keyword(self):
        return keyword_dict[self.value] if self.value in keyword_dict else Keyword.KEYWORD_INVALID

    def to_symbol(self):
        return symbol_dict[self.value] if self.value in symbol_dict else Symbol.SYMBOL_INVALID

    def __str__(self):
        return "{} - {}".format(tokentype_to_str[self.type.value], self.value.replace('\n', '\\n'))

class Tokenizer:
    def __init__(self, r: Reader):
        self.r = r
        self.lookahead: Deque[Token] = deque([])

    def next(self):
        if len(self.lookahead) is 0:
            return self.read()
        else:
            return self.lookahead.popleft()

    def peek(self, amount=0):
        while len(self.lookahead) <= amount:
            self.lookahead.append(self.read())
        return self.lookahead[amount]

    def peek_until(self, *args):
        for token in self.lookahead:
            for arg in args:
                if isinstance(arg, Symbol):
                    if token.to_symbol() == arg:
                        return
                elif isinstance(arg, Keyword):
                    if token.to_keyword() == arg:
                        return
                elif isinstance(arg, TokenType):
                    if token.type == arg:
                        return
                else:
                    if token.value == arg:
                        return

        while True:
            tok = self.read()
            self.lookahead.append(tok)
            for arg in args:
                if isinstance(arg, Symbol):
                    if tok.to_symbol() == arg:
                        return
                elif isinstance(arg, Keyword):
                    if tok.to_keyword() == arg:
                        return
                elif isinstance(arg, TokenType):
                    if tok.type == arg:
                        return
                else:
                    if tok.value == arg:
                        return
            if tok.type == TokenType.EOF:
                return

    def peek_until_no_arrays(self, *args):
        depth = 0
        for token in self.lookahead:
            for arg in args:
                if isinstance(arg, Symbol):
                    if token.to_symbol() == arg and not depth:
                        return
                    elif token.to_symbol() == Symbol.BRACKET_LEFT:
                        depth += 1
                    elif token.to_symbol() == Symbol.BRACKET_RIGHT:
                        depth -= 1
                elif isinstance(arg, Keyword) and not depth:
                    if token.to_keyword() == arg:
                        return
                elif isinstance(arg, TokenType) and not depth:
                    if token.type == arg:
                        return
                else:
                    if token.value == arg and not depth:
                        return

        while True:
            tok = self.read()
            self.lookahead.append(tok)
            for arg in args:
                if isinstance(arg, Symbol):
                    if tok.to_symbol() == arg and not depth:
                        return
                    elif tok.to_symbol() == Symbol.BRACKET_LEFT:
                        depth += 1
                    elif tok.to_symbol() == Symbol.BRACKET_RIGHT:
                        depth -= 1
                elif isinstance(arg, Keyword) and not depth:
                    if tok.to_keyword() == arg:
                        return
                elif isinstance(arg, TokenType) and not depth:
                    if tok.type == arg:
                        return
                else:
                    if tok.value == arg and not depth:
                        return
            if tok.type == TokenType.EOF:
                return

    def search_lookahead(self, *args):
        for k, v in enumerate(self.lookahead):
            for param in args:
                if isinstance(param, Symbol) and v.to_symbol() == param:
                    return k
                elif isinstance(param, Keyword) and v.to_keyword() == param:
                    return k
                elif isinstance(param, TokenType) and v.type == param:
                    return k
                elif v.value == param:
                    return k
        return None


    def read(self):
        letter = r"[a-zA-Z]"
        number = r"[0-9]"
        whitespace = r"[ \n\r\t]"
        symbol = r"[+\-*\/%#~&@|^!?:;<>,.'\"[\]{}()=\\\`]"

        iden_start = r"[^ 0-9\n\r\t+\-*\/%#~&@|^!?:;<>,.'\"[\]{}()=\\\`]"
        iden = r"[^ \n\r\t+\-*\/%#~&@|^!?:;<>,.'\"[\]{}()=\\\`]"

        t = Token('')
        c = self.r.next()

        # Comments and whitespace
        while re.match(whitespace, c) or c == '#' or (c == '/' and (self.r.peek() == '/' or self.r.peek() == '*')):
            if re.match(whitespace, c):
                c = self.r.next()
                while re.match(whitespace, c):
                    c = self.r.next()
            elif c == '#' or (c == '/' and self.r.peek() == '/'):
                while(self.r.next() != '\n' and not self.r.finished):
                    pass # Discard all input
                c = self.r.next()
            else:
                depth = 1 # Nested comments are of importance to me
                while depth > 0 and not self.r.finished:
                    c = self.r.next()
                    while c == '\\':
                        self.r.next()
                        c = self.r.next()
                    if c == '*' and self.r.peek() == '/':
                        self.r.next()
                        depth -= 1
                    elif c == '/' and self.r.peek() == '*':
                        self.r.next()
                        depth += 1
                if self.r.finished and depth > 0:
                    LOGGER.warn("Multiline comment not closed properly")
                c = self.r.next()

        if self.r.finished:
            t.type = TokenType.EOF
            return t

        # Numbers Todo Nice parsing

        if re.match(number, c):
            t.value += c
            while re.match(number, self.r.peek()):
                t.value += self.r.next()

            if self.r.peek() == '.':
                t.value += self.r.next()
                while re.match(number, self.r.peek()):
                    t.value += self.r.next()

            t.type = TokenType.NUMBER
            return t

        # Strings
        if c == '"':
            c = self.r.next()
            while c != '"' and not self.r.finished:
                if c == '\\' and self.r.peek() != '':
                    c = self.r.next()
                    if c == 'n':
                        t.value += '\n'
                    elif c == 't':
                        t.value += '\t'
                    elif c == '0':
                        t.value += '\0'
                    else:
                        t.value += c
                elif c == '':
                    LOGGER.error("String literal extends past end of file")
                else:
                    t.value += c
                c = self.r.next()

            t.type = TokenType.STRING
            return t

        # Char
        if c == "'":
            # Todo Unicode in python? Not very important
            t.value += self.r.next()
            self.r.next() # Remove '
            t.type = TokenType.CHARACTER
            return t

        # Symbol
        if re.match(symbol, c):
            cc = c
            if cc in symbol_dict:
                t.value = cc
                t.type = TokenType.SYMBOL
            while re.match(symbol, self.r.peek()) and not self.r.finished:
                cc += self.r.peek()
                if cc in symbol_dict:
                    t.value = cc
                    t.type = TokenType.SYMBOL
                elif t.type != TokenType.NONE:
                    break
                self.r.next() # Discard peek character

            return t # Might be invalid

        # Compiler Idens
        if c == '$':
            if re.match(iden_start, self.r.peek()):
                t.value += c
                t.value += self.r.next()
                while re.match(iden, self.r.peek()):
                    t.value += self.r.next()
                t.type = TokenType.COMPILER_IDENTIFIER
                return t
            else:
                t.type = TokenType.SYMBOL
                t.value = "$"
                return t

        # Idens and keywords
        if re.match(iden_start, c):
            t.value += c
            while re.match(iden, self.r.peek()) and not self.r.finished:
                t.value += self.r.next()

            if t.value in keyword_dict:
                t.type = TokenType.KEYWORD
            else:
                t.type = TokenType.IDENTIFIER

            return t

        LOGGER.error(c, "Is nothing... ?")

        t.value += c
        return t

