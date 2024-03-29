use std::fmt::Display;

use super::Span;
use crate::util::indexed_vector::{IndexedVec, IndexedVector};

pub type AstId = <IndexedVec<Ast> as IndexedVector>::Index;

#[derive(Debug, PartialEq)]
pub enum AstType {
	/// Top level AST
	Program(AstId),
	/// Placeholder for error
	ErrorAst, // TODO Proper errors

	/// A new name that will create a symbol
	Iden,
	/// A name with a meaning
	Symbol,

	/// A sequence of statements at the file level
	TopBlock(Vec<AstId>),

	/// The body of a function
	FunctionBlock(Vec<AstId>),

	/// A sequence of statements inside a function
	_Block(Vec<AstId>),

	FunctionDefinition {
		ftype: AstId,
		body: AstId,
	},

	NumberLiteral,

	FunctionType {
		name: AstId,
		constant_params: AstId,
		params: AstId,
	},

	Return(AstId),

	Add {
		left: AstId,
		right: AstId,
	},
	FunctionCall {
		function: AstId,
		args: AstId,
	},
	FunctionParams(/* TODO */),
	_FunctionParam(/* TODO */),
}

impl Display for AstType {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match self {
			AstType::Program(b) => write!(f, "Program: ${b}"),
			AstType::ErrorAst => write!(f, "ErrorAst"),
			AstType::Iden => write!(f, "Iden"),
			AstType::Symbol => write!(f, "Symbol"),
			AstType::TopBlock(b) => write!(f, "Top block: {} elements", b.len()),
			AstType::FunctionBlock(b) => write!(f, "Function body: {} elements", b.len()),
			AstType::_Block(b) => write!(f, "Block: {} elements", b.len()),
			AstType::FunctionDefinition { ftype, body } => write!(f, "FunctionDefinition: type ${ftype}, body ${body}"),
			AstType::NumberLiteral => write!(f, "NumberLiteral"),
			AstType::FunctionType {
				name,
				constant_params,
				params,
			} => write!(f, "FunctionType: name ${name}, constant_params: ${constant_params}, params: ${params}"),
			AstType::Return(r) => write!(f, "Return: ${r}"),
			AstType::Add { left, right } => write!(f, "Add: ${left} + ${right}"),
			AstType::FunctionCall { function, args } => write!(f, "FunctionCall: ${function}[](${args})"),
			AstType::FunctionParams() => write!(f, "FunctionParams"),
			AstType::_FunctionParam() => write!(f, "_FunctionParam"),
		}
	}
}

#[derive(Debug)]
pub struct Ast {
	pub id: AstId,
	pub atype: AstType,
	pub span: Span,
}
