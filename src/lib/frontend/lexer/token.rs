use super::Span;

#[derive(Clone, Debug, PartialEq, Eq)]
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
pub struct Token {
	pub span: Span,
	pub ttype: TokenType
}

impl ToString for Token {
	fn to_string(&self) -> String {
		format!("{:?} {}", self.ttype, self.span.to_string())
	}
}

impl Token {
  pub fn is(&self, ttype: TokenType) -> bool {
    self.ttype == ttype
  }
}