use std::fmt::Display;

use crate::util::indexed_vector::{IndexedVec, IndexedVector};

pub type IrId = <IndexedVec<Ir> as IndexedVector>::Index;

pub enum Type {
	U64,
	Fun { params: IrId, returns: IrId },
}

impl Display for Type {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match self {
			Type::U64 => write!(f, "u64"),
			Type::Fun {
				params: p,
				returns: r,
			} => write!(f, "fun[](%{} -> %{})", p, r),
		}
	}
}

pub enum Ir {
	Error(String),
	Reserved,
	Type(Type),
	Iden(String),
	Block(Vec<IrId>),
	Number(u64),
	Value {
		value: IrId,
		r#type: IrId,
	},
	Add {
		lhs: IrId,
		rhs: IrId,
	},
	Return {
		value: IrId,
		block: IrId,
	},
	Decl(String),
	Call {
		fun: IrId,
		args: IrId,
	},
	Deffun {
		iden: IrId,
		// constant_params: IrId,
		params: IrId,
		returns: IrId,
		body: IrId,
	},
}

impl Display for Ir {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match self {
			Ir::Error(s) => write!(f, "Error: {}", s),
			Ir::Reserved => write!(f, "Reserved"),
			Ir::Type(t) => write!(f, "Type: {}", t),
			Ir::Iden(i) => write!(f, "Iden: $\"{}\"", i),
			Ir::Block(v) => write!(f, "Block: {} items", v.len()),
			Ir::Number(n) => write!(f, "Number: {}", n),
			Ir::Value { value, r#type } => write!(f, "Value: %{} as %{}", value, r#type),
			Ir::Add { lhs, rhs } => write!(f, "Add: %{} + %{}", lhs, rhs),
			Ir::Return { value, block } => write!(f, "Return: %{} from %{}", value, block),
			Ir::Decl(i) => write!(f, "Decl: $\"{}\"", i),
			Ir::Call { fun, args } => write!(f, "Call: %{} with %{}", fun, args),
			Ir::Deffun {
				iden,
				params,
				returns,
				body,
			} => write!(
				f,
				"Deffun: {} with params %{}, returns %{} and body %{}",
				iden, params, returns, body
			),
		}
	}
}
