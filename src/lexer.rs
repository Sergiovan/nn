use std::iter::Peekable;
use std::vec::IntoIter;

use crate::module::{module_id, Module};
use crate::compiler::Compiler;

#[derive(Copy, Clone, Debug)]
pub struct Span {
    pub module_idx: module_id,
    pub start: usize,
    pub end: usize
}

impl Span {
    pub fn to_string(&self, c: &Compiler) -> String {
        c.modules[self.module_idx].text_from_span(self)
    }
}

#[derive(Copy, Clone, Debug)]
pub enum TokenType {
    ErrorToken,
    Comment{block: bool}, 
    Iden,
    Integer,
    
    Def,
    Fun,

    Return,

    Add,
    StrongArrowRight,
    OpenParen,
    CloseParen,
    OpenBrace,
    CloseBrace,

    Semicolon,
}

#[derive(Copy, Clone, Debug)]
pub struct Token {
    pub span: Span,
    pub ttype: TokenType
}

impl Token {
    pub fn to_string(&self, c: &Compiler) -> String {
        format!("{:?} {}", self.ttype, self.span.to_string(c))
    }
}

pub struct Lexer {
    module_idx: module_id,
    current: usize,

    data: Peekable<IntoIter<char>>
}

impl Lexer {
    pub fn new(module: &Module) -> Lexer {
        Lexer {
            module_idx: module.id,
            current: 0,
            data: module.source.chars().collect::<Vec<_>>().into_iter().peekable()
        }
    }

    fn peek(&mut self) -> Option<&char> {
        self.data.peek()
    }

    fn next(&mut self) -> Option<char> {
        self.current += 1;
        self.data.next()
    }

    fn number(&mut self, start: usize) -> Token {
        let mut s: String = String::new();

        while let Some(&c) = self.peek() {
            match c {
                '0' ..= '9' => s.push(c),
                _ => break
            }
            self.next();
        }

        Token {
            ttype: TokenType::Integer, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}
        }
    }

    pub fn lex(&mut self) -> Vec<Token> {
        let mut res: Vec<Token> = vec![];

        fn is_whitespace(c: char) -> bool {
            match c {
                ' ' | '\n' | '\t' | '\r' => true,
                _ => false
            }
        }

        fn is_symbol(c: char) -> bool {
            match c {
                '+' | '-' | '*' | '/' | 
                '\\' | '%' | '=' | '!' | 
                '?' | '<' | '>' | '(' | 
                ')' | '{' | '}' | '[' | 
                ']' | ',' | '.' | ':' | 
                ';' | '\'' | '"' | '`' | 
                '@' | '#' | '$' | '&' | 
                '|' | '^' | '~' | '_' => true,
                _ => false
            }
        }

        while let Some(&c) = self.peek() {
            use TokenType as TT;
            let start = self.current;

            let tok: Token = match c {
                '0' ..= '9' => self.number(start),
                '+' => { 
                    self.next();
                    Token {ttype: TT::Add, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                }
                ';' => {
                    self.next();
                    Token {ttype: TT::Semicolon, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                }
                '(' => {
                    self.next();
                    Token {ttype: TT::OpenParen, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                }
                ')' => {
                    self.next();
                    Token {ttype: TT::CloseParen, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                }
                '{' => {
                    self.next();
                    Token {ttype: TT::OpenBrace, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                }
                '}' => {
                    self.next();
                    Token {ttype: TT::CloseBrace, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                }
                '/' => {
                    self.next(); // /
                    if let Some('/') = self.peek() {
                        self.next();

                        while let Some(&c) = self.peek() {
                            self.next(); 
                            match c {
                                '\n' => break,
                                _ => ()
                            }
                        }

                        Token {ttype: TT::Comment{block: false}, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                    } else {
                        Token {ttype: TT::ErrorToken, span: Span{module_idx: self.module_idx, start: start, end: self.current - 1 }}
                    }
                }
                '=' => {
                    self.next(); // =
                    if let Some('>') = self.peek() {
                        self.next(); // >
                        Token {ttype: TT::StrongArrowRight, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                    } else {
                        Token {ttype: TT::ErrorToken, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                    }
                }
                'a' ..= 'z' | 'A' ..= 'Z' | '_' => {
                    let mut s = String::new();
                    while let Some(&c) = self.peek() {
                        if is_whitespace(c) || is_symbol(c) {
                            break;
                        }
                        self.next();
                        s.push(c);
                    }
                    match s.as_str() {
                        "def"    => Token {ttype: TT::Def, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}},
                        "fun"    => Token {ttype: TT::Fun, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}},
                        "return" => Token {ttype: TT::Return, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}},

                        _ => Token {ttype: TT::Iden, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}}
                    }
                }
                ' ' | '\n' | '\t' | '\r' => {
                    self.next();
                    continue;
                }
                _ => {
                    self.next(); // Skip weirdo token
                    Token {ttype: TT::ErrorToken, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1 }}
                }
            };

            res.push(tok);
        };

        res
    } 
}