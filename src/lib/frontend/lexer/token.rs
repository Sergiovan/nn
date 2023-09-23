use super::Span;

#[derive(Clone, Debug, PartialEq)]
pub enum TokenType {
	ErrorToken,
	Eof,

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

#[derive(Clone, Debug)]
pub struct Token<'a> {
	pub span: Span<'a>,
	pub ttype: TokenType
}

impl<'a> ToString for Token<'a> {
	fn to_string(&self) -> String {
		format!("{:?} {}", self.ttype, self.span.to_string())
	}
}