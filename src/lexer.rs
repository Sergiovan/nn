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

#[derive(Clone, Debug, PartialEq)]
pub enum TokenType {
    ErrorToken,
    EOF,

    Comment{block: bool}, 
    Iden(String),
    Integer(u64),
    
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

#[derive(Clone, Debug)]
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

    fn new_token(&self, ttype: TokenType, start: usize) -> Token {
        Token {
            ttype: ttype, span: Span {module_idx: self.module_idx, start: start, end: self.current - 1}
        }
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

        self.new_token(TokenType::Integer(s.parse::<u64>().unwrap()), start)
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
                '|' | '^' | '~' => true,
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
                    self.new_token(TT::Add, start)
                }
                ';' => {
                    self.next();
                    self.new_token(TT::Semicolon, start)
                }
                '(' => {
                    self.next();
                    self.new_token(TT::OpenParen, start)
                }
                ')' => {
                    self.next();
                    self.new_token(TT::CloseParen, start)
                }
                '{' => {
                    self.next();
                    self.new_token(TT::OpenBrace, start)
                }
                '}' => {
                    self.next();
                    self.new_token(TT::CloseBrace, start)
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

                        self.new_token(TT::Comment{block: false}, start)
                    } else {
                        self.new_token(TT::ErrorToken, start)
                    }
                }
                '=' => {
                    self.next(); // =
                    if let Some('>') = self.peek() {
                        self.next(); // >
                        self.new_token(TT::StrongArrowRight, start)
                    } else {
                        self.new_token(TT::ErrorToken, start)
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
                        "def"    => self.new_token(TT::Def, start),
                        "fun"    => self.new_token(TT::Fun, start),
                        "return" => self.new_token(TT::Return, start),

                        _ => self.new_token(TT::Iden(s), start)
                    }
                }
                ' ' | '\n' | '\t' | '\r' => {
                    self.next(); // TODO Whitespace token for formatting purposes
                    continue;
                }
                _ => {
                    self.next(); // Skip weirdo token
                    self.new_token(TT::ErrorToken, start)
                }
            };

            res.push(tok);
        };

        res
    } 
}