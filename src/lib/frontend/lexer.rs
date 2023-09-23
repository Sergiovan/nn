pub mod token;

use self::token::{Token, TokenType};

use std::iter::Peekable;
use std::vec::IntoIter;

use super::module::{Module, span::Span};

// 'l (The content of the Module) will live at least as long as 'm (The reference to the module)
// 'l thus delimits the lifetime of the module, which is longer than the lexer's
pub struct Lexer<'m> {
	module: &'m Module,
	current: u32,
	line: u32, 
	col: u32,

	data: Peekable<IntoIter<char>>
}

impl<'m> Lexer<'m> {
	pub fn new(module: &Module) -> Lexer {
		Lexer {
			module,
			current: 0,
			line: 0,
			col: 0,

			data: module.source().chars().collect::<Vec<_>>().into_iter().peekable()
		}
	}

	fn peek(&mut self) -> Option<char> {
		self.data.peek().copied()
	}

	fn next(&mut self) -> Option<char> {
		self.current += 1;
		self.data.next()
	}

	fn new_token(&self, ttype: TokenType, start: u32) -> Token {
		Token {
			ttype, 
			span: self.module.new_span(start, self.current, self.line, self.col)
		}
	}

	fn number(&mut self, start: u32) -> Token {
		while let Some(c) = self.peek() {
			match c {
				'0' ..= '9' => (),
				_ => break
			}
			self.next();
		}

		self.new_token(TokenType::Integer, start)
	}

	pub fn lex(mut self) -> Vec<Token> {
		let mut res: Vec<Token> = vec![];

		fn is_whitespace(c: char) -> bool {
			matches!(c, ' ' | '\n' | '\t' | '\r')
		}

		fn is_symbol(c: char) -> bool {
			matches!(c, '+' | '-' | '*' | '/' | 
				'\\' | '%' | '=' | '!' | 
				'?' | '<' | '>' | '(' | 
				')' | '{' | '}' | '[' | 
				']' | ',' | '.' | ':' | 
				';' | '\'' | '"' | '`' | 
				'@' | '#' | '$' | '&' | 
				'|' | '^' | '~')
		}

		while let Some(c) = self.peek() {
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

						while let Some(c) = self.peek() {
							self.next(); 
							if c == '\n' {
								break;
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
					while let Some(c) = self.peek() {
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

						_ => self.new_token(TT::Iden, start)
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