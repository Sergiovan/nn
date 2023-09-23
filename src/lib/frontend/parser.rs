pub mod ast;

use self::ast::{Ast, AstId, AstType};

use super::lexer::token::{Token, TokenType};
use super::module::{span::Span, Module};

use crate::util::indexed_vector::{IndexedVec, ivec};

use std::iter::Peekable;
use std::iter::Iterator;
use std::slice;

pub struct ParserError (String);

pub struct Parser<'m> {
	module: &'m Module,
	current: usize,

	data: Peekable<slice::Iter<'m, Token>>,
	asts: IndexedVec<Ast>,
	errors: Vec<ParserError>
}

enum OpType {
	_Prefix,
	Postfix
}

macro_rules! next_is {
	($self:ident, $pat:pat) => {
		(if let Some(Token { ttype: $pat, .. }) = $self.peek() {
			true
		} else {
			false
		})
	}
}

macro_rules! assert_next {
	($self:ident, $pat:pat) => {
		if !next_is!($self, $pat) {
			let current = $self.current;
			let incoming_type = $self.peek().unwrap_or(&$self.eof_token()).ttype.clone();
			panic!("Expected {:?} but found {:?} at position {}", stringify!($pat), incoming_type, current);
		}
	}
}

macro_rules! expect_next {
	($self:ident, $pat:pat) => {
		(if next_is!($self, $pat) {
			true
		} else {
			let current = $self.current;
			let incoming_type = $self.peek().unwrap_or(&$self.eof_token()).ttype.clone();
			$self.new_error(ParserError(format!("Expected {:?} but found {:?} at position {}", stringify!($pat), incoming_type, current)));
			false
		})
	}
}

macro_rules! expect_and_skip {
	($self:ident, $pat:pat) => {
		(if expect_next!($self, $pat) { 
			$self.next(); true 
		} else { 
			false 
		})
	}
}

macro_rules! next_or_error {
	($self:ident) => {
		{
			let Some(e) = $self.next() else {return $self.eof();};
			e
		}
	}
}

macro_rules! peek_or_error {
	($self:ident) => {
		{
			let Some(e) = $self.peek() else {return $self.eof();};
			e
		}
	}
}

use AstType as AT;
use TokenType as TT;

impl<'m> Parser<'m> {
	pub fn new<'a>(module: &'a Module, tokens: Peekable<std::slice::Iter<'a, Token>>) -> Parser<'a> {
		Parser {
			module,
			current: 0,
			data: tokens.clone(),

			asts: ivec![],
			errors: vec![],
		}
	}

	fn eof_token(&self) -> Token {
		let pos: u32 = self.module.source().len() as u32 - 1;
		Token {
				ttype: TT::Eof, 
				span: self.module.new_span(pos, pos, u32::MAX, u32::MAX)
		}
	}

	fn eof(&mut self) -> AstId {
		let eof = self.eof_token();
		self.new_ast(AT::ErrorAst, &eof.span, &eof.span)
	}

	pub fn parse(&mut self) {
		self.program();
	}

	pub fn get_results(self) -> (IndexedVec<Ast>, Vec<ParserError>) {
		(self.asts, self.errors)
	}

	fn new_ast(&mut self, atype: AstType, start: &Span, end: &Span) -> AstId {
		if atype == AT::ErrorAst {
			println!("Error AST!");
		}

		let id: AstId = AstId::from(self.asts.len());

		self.asts.push(Ast {
			id,
			atype, 
			span: self.module.new_span(start.start, end.start + end.text.len() as u32, start.line, start.col)
		});

		id
	}

	fn new_error(&mut self, error: ParserError) {
		self.errors.push(error);
	}

	fn skip_comments(&mut self) {
		while let Some(Token {ttype: TT::Comment { .. }, .. }) = self.peek() {
			self.current += 1;
			self.data.next();
		}
	}

	fn peek(&mut self) -> Option<&'m Token> {
		self.data.peek().cloned()
	}

	fn next(&mut self) -> Option<&'m Token> {
		self.current += 1;
		let res = self.data.next();
		self.skip_comments();
		res
	}

	fn get(&mut self, id: AstId) -> &Ast {
		assert!(self.asts.valid_index(id), "Accessing non-existent ast");
		
		&self.asts[id]
	}

  /** **/

	fn program(&mut self) -> AstId {
		self.skip_comments();

		let t = peek_or_error!(self);
		let start = t.span;
		let block = self.top_block();
		let end = self.asts[block].span;
		self.new_ast(AT::Program(block), &start, &end)

	}

	fn iden(&mut self) -> AstId {
		let n = next_or_error!(self);
		if let Token {span: s, ttype: TT::Iden} = n {
			self.new_ast(AT::Iden, s, s) // TODO extract specific characters?
		} else {
			let t = n.span;
			self.new_ast(AT::ErrorAst, &t, &t)
		}
	}

	fn top_block(&mut self) -> AstId {
		let mut asts = vec![];

		assert!(self.peek().is_some());

		let start = self.peek().unwrap().span;
		let mut end = start;

		loop {
			let t = self.peek();
			let ast = match t {
				None => break,
				Some(_) => {
					self.top_level_statement()
				} 
			};

			asts.push(ast);

			end = self.peek().unwrap_or(&self.eof_token()).span;
		}

		self.new_ast(AT::Block(asts), &start, &end)
	}

	fn top_level_statement(&mut self) -> AstId {
		let t = peek_or_error!(self);

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

	fn function_block(&mut self) -> AstId {
		let mut asts = vec![];

		expect_next!(self, TT::OpenBrace);

		let start = next_or_error!(self).span; // {
		let end = start;

		loop {
			let t = self.peek();
			let ast = match t {
				None => {
					break;
				}
				Some(t) => {
					if t.is(TT::CloseBrace) {
						break;
					} else {
						self.statement()
					}
				}
			};

			asts.push(ast);
		}

		expect_and_skip!(self, TT::CloseBrace); // }

		self.new_ast(AT::Block(asts), &start, &end)
	}

	fn statement(&mut self) -> AstId {
		let t = peek_or_error!(self);

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

	fn define(&mut self) -> AstId {
		assert_next!(self, TT::Def);

		let def = self.next().unwrap(); // def
		let def_span = def.span;

		let t = self.peek();
		match t {
			None => self.new_ast(AT::ErrorAst, &def_span, &def_span),
			Some(t) => {
				if t.is(TT::Fun) {
					let ftype = self.function_type();
					// TODO Expect body?
					let (fbody, expr) = self.function_body(); // TODO No body
	
					let _ = expr && expect_and_skip!(self, TT::Semicolon);
	
					let start_span = self.get(ftype).span;
					let end_span = self.get(fbody).span;
	
					self.new_ast(AT::FunctionDefinition{ftype, body: fbody}, &start_span, &end_span)
				} else {
					let end_span = t.span;
					self.new_ast(AT::ErrorAst, &def_span, &end_span)
				}
			}
		}
	}

	fn function_type(&mut self) -> AstId {
		assert_next!(self, TT::Fun);

		let fun = self.next().unwrap(); // fun
		let fun_span = fun.span;

		expect_next!(self, TT::Iden);
		let name = self.iden();

		expect_next!(self, TT::OpenParen); // TODO Constant parameters
		let params = self.function_params();

		let end_span = self.get(params).span;
		self.new_ast(AT::FunctionType{name, constant_params: AstId::from(u32::MAX), params}, &fun_span, &end_span)
	}

	fn function_params(&mut self) -> AstId {
		assert_next!(self, TT::OpenParen);

		let oparen = self.next().unwrap(); // (
		let start_span = oparen.span;

		assert!(oparen.ttype == TT::OpenParen);

		// TODO Parameters :P
		let cparen = self.peek().unwrap(); // TODO )
		let end_span = cparen.span;
		expect_and_skip!(self, TT::CloseParen);

		self.new_ast(AT::FunctionParams(), &start_span, &end_span)
	}

	fn function_body(&mut self) -> (AstId, bool) {
		let t = self.peek().unwrap(); // TODO fix
		let error_span = t.span;

		match t.ttype {
			TT::OpenBrace => {
				(self.function_block(), false)
			}
			TT::StrongArrowRight => {
				let arrow = self.next().unwrap(); // TODO fix =>
				let start_span = arrow.span;

				let expr = self.expression();
				let expr_span = self.get(expr).span;

				let retexpr = self.new_ast(AT::Return(expr), &expr_span, &expr_span);

				(self.new_ast(
					AT::Block(vec![retexpr]), &start_span, &expr_span
				), true)
			}
			TT::Eof => {
				(self.new_ast(AT::ErrorAst, &error_span, &error_span), false)
			}
			_ => {
				(self.new_ast(AT::ErrorAst, &error_span, &error_span), false)
			}
		}
	}

	fn expression(&mut self) -> AstId {
		let t = peek_or_error!(self);

		if t.is(TT::Return) {
			self.return_expression()
		} else {
			let res = self.value_expression(0);
			expect_and_skip!(self, TT::Semicolon);
			res
		}
	}

	fn return_expression(&mut self) -> AstId {
		assert_next!(self, TT::Return);

		let r#return = self.next().unwrap(); // return
		let return_span = r#return.span;

		let res = self.expression();
		let res_span = self.get(res).span;
		self.new_ast(AT::Return(res), &r#return_span, &res_span)
	}

	fn operator_precedence(ttype: &TokenType, op_type: OpType) -> usize {
		match ttype {
			TT::Add => match op_type {
				OpType::_Prefix => 90,
				OpType::Postfix => 30
			},
			TT::OpenParen => match op_type {
				OpType::_Prefix => 0xFFFFFFFF,
				OpType::Postfix => 50
			}
			_ => 0
		}
	}

	fn value_expression(&mut self, precedence: usize) -> AstId {
		let tok = peek_or_error!(self);

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
			TT::Integer => {
				let tok_span = tok.span;
				self.next(); // Number
				self.new_ast(AT::NumberLiteral, &tok_span, &tok_span)
			}
			TT::Iden => {
				self.iden()
			}
			_ => {
				let tok_span = tok.span;
				self.next(); // ???
				self.new_ast(AT::ErrorAst, &tok_span, &tok_span)
			}
		};

		loop {
			let tok = peek_or_error!(self);
			let new_precedence = Self::operator_precedence(&tok.ttype, OpType::Postfix);
			if new_precedence < precedence {
				return res;
			}

			res = match tok.ttype {
				TT::OpenParen => {
					self.next(); // (
					// TODO Params
					let close = peek_or_error!(self); 
					let close_span = close.span;
					let res_span = self.get(res).span;

					expect_and_skip!(self, TT::CloseParen);
					self.new_ast(AT::FunctionCall { function: res, params: AstId::from(u32::MAX)}, &res_span, &close_span)
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