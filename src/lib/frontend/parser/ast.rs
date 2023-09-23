use crate::util::indexed_vector::{IndexedVec, IndexedVector};
use super::Span;

pub type AstId = <IndexedVec<Ast<'static>> as IndexedVector>::Index;

#[derive(Debug, PartialEq)]
pub enum AstType {
	Program(AstId),
	ErrorAst, // TODO Proper errors

	Iden,
	Block(Vec<AstId>),
	FunctionDefinition{ftype: AstId, body: AstId},

	NumberLiteral,

	FunctionType{name: AstId, constant_params: AstId, params: AstId},
	
	Return(AstId),
	
	Add{left: AstId, right: AstId},
	FunctionCall{function: AstId, params: AstId},
	FunctionParams( /* TODO */ ),
	_FunctionParam( /* TODO */ )
}

#[derive(Debug)]
pub struct Ast<'a> {
	pub id: AstId,
	pub atype: AstType,
	pub span: Span<'a>
}