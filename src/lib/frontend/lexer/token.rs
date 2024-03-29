use std::ops::{Deref, DerefMut};

use super::Span;

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum TokenType {
	ErrorToken,
	Eof,

	Comment { block: bool },
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

#[derive(Clone, Debug)]
pub struct Token {
	pub span: Span,
	pub ttype: TokenType,
}

impl std::fmt::Display for Token {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		write!(f, "{:?} {}", self.ttype, self.span)
	}
}

impl Token {
	pub fn is(&self, ttype: TokenType) -> bool {
		self.ttype == ttype
	}

	pub fn _one_of(&self, ttypes: &[TokenType]) -> bool {
		ttypes.iter().any(|tt| &self.ttype == tt)
	}

	pub fn _nok(&self) -> bool {
		matches!(self.ttype, TokenType::ErrorToken | TokenType::Eof)
	}
}

#[derive(Debug)]
pub struct MaybeToken<'m>(Option<&'m Token>);

impl<'a> MaybeToken<'a> {
	pub fn assert(self) -> &'a Token {
		if self.0.is_none() {
			panic!("Must have a token, found nothing");
		} else {
			self.0.unwrap()
		}
	}

	pub fn assert_is(self, ttype: TokenType) -> &'a Token {
		let t = self.assert();
		if !t.is(ttype) {
			panic!("Must have {:?}, found {:?} instead", ttype, t.ttype);
		}

		t
	}

	pub fn _assert_one_of(self, ttypes: &[TokenType]) -> &'a Token {
		let t = self.assert();
		if !t._one_of(ttypes) {
			panic!("Must have one of {:?}, found {:?} instead", ttypes, t.ttype);
		}

		t
	}

	pub fn expect(self) -> Result<&'a Token, Option<&'a Token>> {
		self.0.ok_or(self.0)
	}

	pub fn expect_is(self, ttype: TokenType) -> Result<&'a Token, Option<&'a Token>> {
		self.0.and_then(|t| t.is(ttype).then_some(t)).ok_or(self.0)
	}

	pub fn _expect_one_of(self, ttypes: &[TokenType]) -> Result<&'a Token, Option<&'a Token>> {
		self.0
			.and_then(|t| t._one_of(ttypes).then_some(t))
			.ok_or(self.0)
	}

	pub fn is(&self, ttype: TokenType) -> bool {
		self.0.is_some_and(|t| t.ttype == ttype)
	}

	pub fn _one_of(&self, ttypes: &[TokenType]) -> bool {
		self.0.is_some_and(|t| t._one_of(ttypes))
	}

	pub fn ttype(&self) -> TokenType {
		self.0.map(|t| t.ttype).unwrap_or(TokenType::Eof)
	}
}

impl<'a> From<Option<&'a Token>> for MaybeToken<'a> {
	fn from(value: Option<&'a Token>) -> Self {
		MaybeToken(value)
	}
}

impl<'a> Deref for MaybeToken<'a> {
	type Target = Option<&'a Token>;

	fn deref(&self) -> &Self::Target {
		&self.0
	}
}

impl<'a> DerefMut for MaybeToken<'a> {
	fn deref_mut(&mut self) -> &mut Self::Target {
		&mut self.0
	}
}
