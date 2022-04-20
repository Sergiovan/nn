use crate::{lexer, ast};
use lexer::{LexInfo, Token};
use ast::Ast;

pub struct Parser {
    tokens: Vec<Token>,
    i: usize,
}

use lexer::TokenData as T;

impl Parser {
    pub fn new(toks: &LexInfo) -> Parser {
        Parser {
            tokens: toks.sanitized(),
            i: 0
        }
    }

    fn peek(&self) -> Option<Token> {
        if self.i >= self.tokens.len() {
            None
        } else {
            Some(self.tokens[self.i].clone())
        }
    }

    fn peek_many(&self, amount: usize) -> Option<Token> {
        if self.i + amount >= self.tokens.len() {
            None
        } else {
            Some(self.tokens[self.i + amount].clone())
        }
    }

    fn next(&mut self) -> Option<Token> {
        let res;
        if self.i > self.tokens.len() {
            res = None;
        } else {
            res = Some(self.tokens[self.i].clone());
        }
        self.i += 1;
        res
    }

    pub fn parse_program(&mut self) -> Ast {
        let mut stmts = Vec::<Ast>::new();
        
        while let Some(c) = self.peek() {
            stmts.push(match c.t {
                T::IMPORT => {
                    self.importstmt()
                }
                T::USING => {
                    self.usingstmt()
                }
                T::DEF => {
                    self.defstmt()
                }
                T::VAR | T::CONST => {
                    self.freevarstmt()
                }
                T::EXTERN => {
                    self.externstmt()
                }
                _ => {
                    self.next(); // Unknown thing
                    Ast::ERROR(format!(r#"Expected "import", "using", "def", "var", "const" or "extern", but got {:?} instead"#, c))
                }
            });
        }

        Ast::PROGRAM(stmts)
    }

    fn importstmt(&mut self) -> Ast {
        self.next(); // import
        
    }

    fn usingstmt(&mut self) -> Ast {
        self.next();
        Ast::ERROR("Not implemented".to_string())
    }

    fn defstmt(&mut self) -> Ast {
        self.next();
        Ast::ERROR("Not implemented".to_string())
    }

    fn freevarstmt(&mut self) -> Ast {
        self.next();
        Ast::ERROR("Not implemented".to_string())
    }

    fn externstmt(&mut self) -> Ast {
        self.next();
        Ast::ERROR("Not implemented".to_string())
    }
}