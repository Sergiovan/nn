pub mod ast;

use self::ast::{Ast, AstId, AstType};

use super::lexer::token::{MaybeToken, Token, TokenType};
use super::module::{span::Span, Module};

use crate::util::indexed_vector::{ivec, IndexedVec};

use std::iter::Iterator;
use std::iter::Peekable;
use std::slice;

#[derive(Debug)]
pub struct ParserError(String);

pub struct Parser<'m> {
	module: &'m Module,
	current: usize,

	data: Peekable<slice::Iter<'m, Token>>,
	asts: IndexedVec<Ast>,
	errors: Vec<ParserError>,
}

enum OpType {
	_Prefix,
	Postfix,
}

use AstType as AT;
use TokenType as TT;

type ParserResult<T = AstId> = Result<T, ()>;

impl<'m> Parser<'m> {
	pub fn new<'a>(
		module: &'a Module,
		tokens: Peekable<std::slice::Iter<'a, Token>>,
	) -> Parser<'a> {
		Parser {
			module,
			current: 0,
			data: tokens.clone(),

			asts: ivec![],
			errors: vec![],
		}
	}

	pub fn parse(&mut self) {
		match self.program() {
			Ok(_) => (),
			Err(()) => {
				self.new_error(ParserError(format!(
					"Program ended unexpectedly at {}",
					self.current
				)));
			}
		};
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
			span: self.module.new_span(
				start.start,
				end.start + end.text.len() as u32,
				start.line,
				start.col,
			),
		});

		id
	}

	fn new_error_ast(&mut self) -> AstId {
		println!("Error AST!");

		let id: AstId = AstId::from(self.asts.len());

		let span = self.peek().unwrap_or(self.module.eof_token()).span;

		self.asts.push(Ast {
			id,
			atype: AT::ErrorAst,
			span,
		});

		id
	}

	fn new_error(&mut self, error: ParserError) {
		self.errors.push(error);
	}

	fn skip_comments(&mut self) {
		while let Ok(Token {
			ttype: TT::Comment { .. },
			..
		}) = self.peek()
		{
			self.current += 1;
			self.data.next();
		}
	}

	fn raw_peek(&mut self) -> MaybeToken<'m> {
		self.data.peek().cloned().into()
	}

	fn raw_next(&mut self) -> MaybeToken<'m> {
		self.current += 1;
		let res = self.data.next();
		self.skip_comments();
		res.into()
	}

	fn peek(&mut self) -> ParserResult<&'m Token> {
		self.raw_peek().expect().map_err(|_| ())
	}

	fn next(&mut self) -> ParserResult<&'m Token> {
		self.raw_next().expect().map_err(|_| ())
	}

	fn skip(&mut self) {
		self.raw_next();
	}

	fn _peek_assert(&mut self, ttype: TokenType) -> &'m Token {
		self.raw_peek().assert_is(ttype)
	}

	fn next_assert(&mut self, ttype: TokenType) -> &'m Token {
		self.raw_next().assert_is(ttype)
	}

	fn peek_expect(&mut self, ttype: TokenType) -> ParserResult<&'m Token> {
		self.raw_peek().expect_is(ttype).map_err(|e| {
			let t = e.unwrap_or(self.module.eof_token());
			self.new_error(ParserError(format!(
				"Expected {:?} but found {:?} at position {}",
				ttype, t.ttype, self.current
			)));
		})
	}

	fn next_expect(&mut self, ttype: TokenType) -> ParserResult<&'m Token> {
		self.raw_next().expect_is(ttype).map_err(|e| {
			let t = e.unwrap_or(self.module.eof_token());
			self.new_error(ParserError(format!(
				"Expected {:?} but found {:?} at position {}",
				ttype, t.ttype, self.current
			)));
		})
	}

	fn peek_is(&mut self, ttype: TokenType) -> bool {
		self.raw_peek().is(ttype)
	}

	fn _peek_one_of(&mut self, ttypes: &[TokenType]) -> bool {
		self.raw_peek()._one_of(ttypes)
	}

	fn skip_if_is(&mut self, ttype: TokenType) -> bool {
		if self.peek_is(ttype) {
			self.raw_next();
			true
		} else {
			false
		}
	}

	fn _skip_if_one_of(&mut self, ttypes: &[TokenType]) -> bool {
		if self._peek_one_of(ttypes) {
			self.raw_next();
			true
		} else {
			false
		}
	}

	fn expect_is_and_skip(&mut self, ttype: TokenType) -> bool {
		if !self.skip_if_is(ttype) {
			let expected = self.raw_peek().ttype();
			self.new_error(ParserError(format!(
				"Expected {:?} but found {:?} at position {}",
				ttype, expected, self.current
			)));
			false
		} else {
			true
		}
	}

	fn get(&mut self, id: AstId) -> &Ast {
		assert!(self.asts.valid_index(id), "Accessing non-existent ast");

		&self.asts[id]
	}

	/** **/

	fn program(&mut self) -> ParserResult {
		self.skip_comments();

		let t = self.peek()?;
		let start = t.span;
		let block = self.top_block()?;
		let end = self.asts[block].span;
		Ok(self.new_ast(AT::Program(block), &start, &end))
	}

	fn iden(&mut self, is_symbol: bool) -> ParserResult {
		let n = self.next()?;
		if let Token {
			span: s,
			ttype: TT::Iden,
		} = n
		{
			if is_symbol {
				Ok(self.new_ast(AT::Symbol, s, s))
			} else {
				Ok(self.new_ast(AT::Iden, s, s)) // TODO extract specific characters?
			}
		} else {
			let t = n.span;
			Ok(self.new_ast(AT::ErrorAst, &t, &t))
		}
	}

	fn top_block(&mut self) -> ParserResult {
		let mut asts = vec![];

		let t = self.raw_peek().assert();

		let start = t.span;
		let mut end = start;

		loop {
			let t = self.peek();
			if t.is_err() {
				break;
			}
			let ast = self.top_level_statement()?;

			asts.push(ast);

			end = self.peek().unwrap_or(self.module.eof_token()).span;
		}

		Ok(self.new_ast(AT::TopBlock(asts), &start, &end))
	}

	fn top_level_statement(&mut self) -> ParserResult {
		let t = self.peek()?;

		if t.is(TT::Def) {
			Ok(self.define()?)
		} else {
			let t_span = t.span;
			let r = self.new_ast(AT::ErrorAst, &t_span, &t_span);
			self.skip();
			Ok(r)
		}
	}

	fn function_block(&mut self) -> ParserResult {
		let mut asts = vec![];

		let Ok(t) = self.next_expect(TT::OpenBrace) else {
			return Ok(self.new_error_ast());
		}; // {

		let start = t.span;
		let end = start;

		loop {
			let t = self.peek()?;
			if t.is(TT::CloseBrace) {
				break;
			}
			let ast = self.statement()?;

			asts.push(ast);
		}

		self.expect_is_and_skip(TT::CloseBrace); // }

		Ok(self.new_ast(AT::FunctionBlock(asts), &start, &end))
	}

	fn statement(&mut self) -> ParserResult {
		let t = self.peek()?;

		match t.ttype {
			TT::Def => self.define(),
			_ => {
				let res = self.expression()?; // Easy tail recursion hmm
				self.expect_is_and_skip(TT::Semicolon);
				Ok(res)
			}
		}
	}

	fn define(&mut self) -> ParserResult {
		let def = self.next_assert(TT::Def); // def

		let t = self.peek()?;

		match t.ttype {
			TT::Fun => {
				let ftype = self.function_type()?;
				// TODO Expect body?
				let fbody = self.function_body()?; // TODO No body

				let start_span = def.span;
				let end_span = self.get(fbody).span;

				Ok(self.new_ast(
					AT::FunctionDefinition { ftype, body: fbody },
					&start_span,
					&end_span,
				))
			}
			_ => Ok(self.new_error_ast()),
		}
	}

	fn function_type(&mut self) -> ParserResult {
		let fun = self.next_assert(TT::Fun); // fun
		let fun_span = fun.span;

		if self.peek_expect(TT::Iden).is_err() {
			return Ok(self.new_error_ast());
		};

		let name = self.iden(false)?;

		// TODO Constant parameters

		if self.peek_expect(TT::OpenParen).is_err() {
			return Ok(self.new_error_ast());
		}

		let params = self.function_params()?;

		let end_span = self.get(params).span;
		Ok(self.new_ast(
			AT::FunctionType {
				name,
				constant_params: u32::MAX.into(),
				params,
			},
			&fun_span,
			&end_span,
		))
	}

	fn function_params(&mut self) -> ParserResult {
		let oparen = self.next_assert(TT::OpenParen); // (
		let start_span = oparen.span;

		// TODO Parameters :P
		let cparen = self.peek_expect(TT::CloseParen); // )
		let end_span = cparen.unwrap_or(self.peek()?).span;
		self.skip_if_is(TT::CloseParen);

		Ok(self.new_ast(AT::FunctionParams(), &start_span, &end_span))
	}

	fn function_body(&mut self) -> ParserResult {
		let t = self.peek()?;
		let error_span = t.span;

		match t.ttype {
			TT::OpenBrace => Ok(self.function_block()?),
			TT::StrongArrowRight => {
				let arrow = self.next().unwrap(); // TODO fix =>
				let start_span = arrow.span;

				let expr = self.expression()?;
				let expr_span = self.get(expr).span;

				self.expect_is_and_skip(TT::Semicolon);

				let retexpr = self.new_ast(AT::Return(expr), &expr_span, &expr_span);

				Ok(self.new_ast(AT::FunctionBlock(vec![retexpr]), &start_span, &expr_span))
			}
			_ => Ok(self.new_ast(AT::ErrorAst, &error_span, &error_span)),
		}
	}

	fn expression(&mut self) -> ParserResult {
		let t = self.peek()?;

		if t.is(TT::Return) {
			self.return_expression()
		} else {
			self.value_expression(0)
		}
	}

	fn return_expression(&mut self) -> ParserResult {
		let r#return = self.next_assert(TT::Return); // return
		let return_span = r#return.span;

		let res = self.expression()?;
		let res_span = self.get(res).span;
		Ok(self.new_ast(AT::Return(res), &r#return_span, &res_span))
	}

	fn operator_precedence(ttype: &TokenType, op_type: OpType) -> usize {
		match ttype {
			TT::Add => match op_type {
				OpType::_Prefix => 90,
				OpType::Postfix => 30,
			},
			TT::OpenParen => match op_type {
				OpType::_Prefix => 0xFFFFFFFF,
				OpType::Postfix => 50,
			},
			_ => 0,
		}
	}

	fn value_expression(&mut self, precedence: usize) -> ParserResult {
		let tok = self.peek()?;

		// Prefixes
		let mut res = match &tok.ttype {
			// TT::Negate => {
			//     self.next(); // -
			//     let expr = self.value_expression(Self::operator_precedence(TT::Negate, OpType::Prefix));
			//     self.new_ast(AT::Negate(expr), &tok.span, &self.get(expr).span)
			// }
			TT::OpenParen => {
				self.skip(); // (
				let expr = self.expression()?;
				self.expect_is_and_skip(TT::CloseParen); // )
				expr
			}
			TT::Integer => {
				let tok_span = tok.span;
				self.skip(); // Integer
				self.new_ast(AT::NumberLiteral, &tok_span, &tok_span)
			}
			TT::Iden => self.iden(true)?,
			_ => {
				let tok_span = tok.span;
				self.skip(); // ???
				self.new_ast(AT::ErrorAst, &tok_span, &tok_span)
			}
		};

		loop {
			let tok = self.peek()?;
			let new_precedence = Self::operator_precedence(&tok.ttype, OpType::Postfix);
			if new_precedence < precedence {
				return Ok(res);
			}

			res = match tok.ttype {
				TT::OpenParen => {
					// Function call
					self.skip(); // (

					// TODO Function Params
					let close = self.peek()?;
					let close_span = close.span;
					let res_span = self.get(res).span;

					self.expect_is_and_skip(TT::CloseParen); // )

					self.new_ast(
						AT::FunctionCall {
							function: res,
							args: u32::MAX.into(),
						},
						&res_span,
						&close_span,
					)
				}
				TT::Add => {
					self.skip(); // +
					let rhs = self.value_expression(new_precedence)?;
					let start_span = self.get(res).span;
					let end_span = self.get(rhs).span;
					self.new_ast(
						AT::Add {
							left: res,
							right: rhs,
						},
						&start_span,
						&end_span,
					)
				}
				_ => {
					return Ok(res);
				}
			}
		}
	}
}
