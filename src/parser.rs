use std::iter::Peekable;
use std::iter::Iterator;
use std::slice;

use crate::lexer::{Span, Token, TokenType};
use crate::module::{Module, module_id};

pub type ast_id = usize;

#[derive(Debug, PartialEq)]
pub enum AstType {
    Program(ast_id),
    ErrorAst, // TODO Proper errors

    Iden(String),
    Block(Vec<ast_id>),
    FunctionDefinition{ftype: ast_id, body: ast_id},

    NumberLiteral(u64),

    FunctionType{name: ast_id, constant_params: ast_id, params: ast_id},
    
    Return(ast_id),
    
    Add{left: ast_id, right: ast_id},
    FunctionCall{function: ast_id, params: ast_id},
    FunctionParams( /* TODO */ ),
    FunctionParam( /* TODO */ )
}

#[derive(Debug)]
pub struct Ast {
    pub id: ast_id,
    pub atype: AstType,
    pub span: Span
}

pub struct ParserError (String);

pub struct Parser<'a> {
    module_idx: module_id,
    current: usize,

    data: Peekable<slice::Iter<'a, Token>>,
    asts: Vec<Ast>,
    errors: Vec<ParserError>,

    eof_token: Token
}

enum OpType {
    Prefix,
    Postfix
}

macro_rules! next_is {
    ($self:ident, $pat:pat) => {
        (if let $pat = $self.peek().ttype {
            true
        } else {
            false
        })
    }
}

macro_rules! expect {
    ($self:ident, $pat:pat) => {
        (if next_is!($self, $pat) {
            true
        } else {
            let current = $self.current;
            let incoming_type = $self.peek().ttype.clone();
            $self.new_error(ParserError(format!("Expected {:?} but found {:?} at position {}", stringify!($pat), incoming_type, current)));
            false
        })
    }
}

macro_rules! expect_and_skip {
    ($self:ident, $pat:pat) => {
        (if expect!($self, $pat) { 
            $self.next(); true 
        } else { 
            false 
        })
    }
}

use AstType as AT;
use TokenType as TT;
impl<'a> Parser<'a> {
    pub fn new(module: &Module) -> Parser {
        Parser {
            module_idx: module.id,
            current: 0,
            data: module.tokens.iter().peekable(),

            asts: vec![],
            errors: vec![],

            eof_token: Token {ttype: TT::EOF, span: Span { module_idx: module.id, start: module.source.len() - 1, end: module.source.len() - 1}}
        }
    }

    pub fn parse(&mut self) {
        self.program();
    }

    pub fn get_results(self) -> (Vec<Ast>, Vec<ParserError>) {
        (self.asts, self.errors)
    }

    fn new_ast(&mut self, atype: AstType, start: &Span, end: &Span) -> ast_id {
        if atype == AT::ErrorAst {
            println!("Error AST!");
        }

        let id: ast_id = self.asts.len();

        self.asts.push(Ast {
            id: id,
            atype: atype, 
            span: Span { 
                module_idx: start.module_idx,
                start: start.start,
                end: end.end
            }
        });

        id
    }

    fn new_error(&mut self, error: ParserError) {
        self.errors.push(error);
    }

    fn skip_comments(&mut self) {
        while let Token {ttype: TT::Comment { .. }, .. } = self.peek() {
            self.current += 1;
            self.data.next();
        }
    }

    fn peek(&mut self) -> &Token {
        if let Some(&tok) = self.data.peek() {
            tok
        } else {
            &self.eof_token
        }
    }

    fn next(&mut self) -> &Token {
        self.current += 1;
        let res = self.data.next();
        self.skip_comments();
        res.unwrap_or(&self.eof_token)
    }

    fn get(&mut self, id: ast_id) -> &Ast {
        assert!(id < self.asts.len(), "Accessing non-existent ast");
        
        &self.asts[id]
    }

    fn program(&mut self) -> ast_id {
        self.skip_comments();

        let t = self.peek();
        if let t @ Token {ttype: TT::EOF, ..} = t {
            let madeup = Span {module_idx: self.module_idx, start: 0, end: 0};
            self.new_ast(AT::ErrorAst, &madeup, &madeup)
        } else {
            let start = t.span;
            let block = self.top_block();
            let end = self.asts[block].span;
            self.new_ast(AT::Program(block), &start, &end)
        }
    }

    fn iden(&mut self) -> ast_id {
        let iden = self.next();
        if let TT::Iden(s) = &iden.ttype {
            let t = iden.span;
            let str = s.clone();
            self.new_ast(AT::Iden(str), &t, &t) // TODO extract specific characters?
        } else {
            let t = self.next().span;
            self.new_ast(AT::ErrorAst, &t, &t)
        }
    }

    fn top_block(&mut self) -> ast_id {
        let mut asts = vec![];

        assert!(self.peek().ttype != TT::EOF);

        let start = self.peek().span;
        let mut end = start;

        loop {
            let t = self.peek();
            let ast = match t.ttype {
                TT::EOF => break,
                _ => {
                    self.top_level_statement()
                } 
            };

            asts.push(ast);

            end = self.peek().span;
        }

        self.new_ast(AT::Block(asts), &start, &end)
    }

    fn top_level_statement(&mut self) -> ast_id {
        let t = self.peek();
        match t.ttype {
            TT::Def => {
                self.define()
            }
            _ => {
                let t_span = t.span;
                let r = self.new_ast(AT::ErrorAst, &t_span, &t_span);
                self.next();
                r
            }
        }
    }

    fn function_block(&mut self) -> ast_id {
        let mut asts = vec![];

        expect!(self, TT::OpenBrace);

        let start = self.next().span; // {
        let end = start;

        loop {
            let t = self.peek();
            let ast = match t.ttype {
                TT::CloseBrace => {
                    break;
                }
                TT::EOF => {
                    break;
                }
                _ => {
                    self.statement()
                }
            };

            asts.push(ast);
        }

        expect_and_skip!(self, TT::CloseBrace);

        self.new_ast(AT::Block(asts), &start, &end)
    }

    fn statement(&mut self) -> ast_id {
        let t = self.peek();

        match t.ttype {
            TT::Def => {
                self.define()
            }
            _ => {
                let res = self.expression(); // Easy tail recursion hmm
                expect_and_skip!(self, TT::Semicolon);
                res
            }
        }
    }

    fn define(&mut self) -> ast_id {
        let def = self.next(); // def
        let def_span = def.span;
        
        assert!(def.ttype == TT::Def);

        let t = self.peek();
        match t.ttype {
            TT::Fun => {
                let ftype = self.function_type();
                // TODO Expect body?
                let (fbody, expr) = self.function_body(); // TODO No body

                let _ = expr && expect_and_skip!(self, TT::Semicolon);

                let start_span = self.get(ftype).span;
                let end_span = self.get(fbody).span;

                self.new_ast(AT::FunctionDefinition{ftype: ftype, body: fbody}, &start_span, &end_span)
            }
            TT::EOF => self.new_ast(AT::ErrorAst, &def_span, &def_span),
            _ => {
                let end_span = t.span;
                self.new_ast(AT::ErrorAst, &def_span, &end_span)
            }
        }
    }

    fn function_type(&mut self) -> ast_id {
        let fun = self.next(); // fun
        let fun_span = fun.span;

        assert!(fun.ttype == TT::Fun);

        expect!(self, TT::Iden(_));
        let name = self.iden();

        expect!(self, TT::OpenParen); // TODO Constant parameters
        let params = self.function_params();

        let end_span = self.get(params).span;
        self.new_ast(AT::FunctionType{name: name, constant_params: usize::MAX, params: params}, &fun_span, &end_span)
    }

    fn function_params(&mut self) -> ast_id {
        let oparen = self.next(); // (
        let start_span = oparen.span;

        assert!(oparen.ttype == TT::OpenParen);

        // TODO Parameters :P
        let cparen = self.peek();
        let end_span = cparen.span;
        expect_and_skip!(self, TT::CloseParen);

        self.new_ast(AT::FunctionParams(), &start_span, &end_span)
    }

    fn function_body(&mut self) -> (ast_id, bool) {
        let t = self.peek();
        let error_span = t.span;
        match t.ttype {
            TT::OpenBrace => {
                (self.function_block(), false)
            }
            TT::StrongArrowRight => {
                let arrow = self.next(); // =>
                let start_span = arrow.span;

                let expr = self.expression();
                let expr_span = self.get(expr).span;

                let retexpr = self.new_ast(AT::Return(expr), &expr_span, &expr_span);

                (self.new_ast(
                    AT::Block(vec![retexpr]), &start_span, &expr_span
                ), true)
            }
            TT::EOF => {
                (self.new_ast(AT::ErrorAst, &error_span, &error_span), false)
            }
            _ => {
                (self.new_ast(AT::ErrorAst, &error_span, &error_span), false)
            }
        }
    }

    fn expression(&mut self) -> ast_id {
        let t = self.peek();

        match t.ttype {
            TT::Return => {
                self.return_expression()
            }
            _ => {
                let res = self.value_expression(0);
                expect_and_skip!(self, TT::Semicolon);
                res
            }
        }
    }

    fn return_expression(&mut self) -> ast_id {
        let r#return = self.next(); // return
        let return_span = r#return.span;

        assert!(r#return.ttype == TT::Return);

        let res = self.expression();
        let res_span = self.get(res).span;
        self.new_ast(AT::Return(res), &r#return_span, &res_span)
    }

    fn operator_precedence(ttype: &TokenType, op_type: OpType) -> usize {
        match ttype {
            TT::Add => match op_type {
                OpType::Prefix => 90,
                OpType::Postfix => 30
            },
            TT::OpenParen => match op_type {
                OpType::Prefix => 0xFFFFFFFF,
                OpType::Postfix => 50
            }
            _ => 0
        }
    }

    fn value_expression(&mut self, precedence: usize) -> ast_id {
        let tok = self.peek();

        // Prefixes
        let mut res = match &tok.ttype {
            // TT::Negate => {
            //     self.next(); // -
            //     let expr = self.value_expression(Self::operator_precedence(TT::Negate, OpType::Prefix));
            //     self.new_ast(AT::Negate(expr), &tok.span, &self.get(expr).span)
            // }
            TT::OpenParen => {
                self.next(); // (
                let expr = self.expression();
                expect_and_skip!(self, TT::CloseParen); // )
                expr
            }
            TT::Integer(i) => {
                let tok_span = tok.span;
                let num = *i;
                self.next(); // Number
                self.new_ast(AT::NumberLiteral(num), &tok_span, &tok_span)
            }
            TT::Iden(_) => {
                self.iden()
            }
            _ => {
                let tok_span = tok.span;
                self.next(); // ???
                self.new_ast(AT::ErrorAst, &tok_span, &tok_span)
            }
        };

        loop {
            let tok = self.peek();
            let new_precedence = Self::operator_precedence(&tok.ttype, OpType::Postfix);
            if new_precedence < precedence {
                return res;
            }

            res = match tok.ttype {
                TT::OpenParen => {
                    self.next(); // (
                    // TODO Params
                    let close = self.peek(); 
                    let close_span = close.span;
                    let res_span = self.get(res).span;

                    expect_and_skip!(self, TT::CloseParen);
                    self.new_ast(AT::FunctionCall { function: res, params: 0xFFFFFFFF }, &res_span, &close_span)
                }
                TT::Add => {
                    self.next(); // +
                    let rhs = self.value_expression(new_precedence);
                    let start_span = self.get(res).span;
                    let end_span = self.get(rhs).span;
                    self.new_ast(AT::Add{left: res, right: rhs}, &start_span, &end_span)
                }
                _ => {
                    return res;
                }
            }
        }
    }
}