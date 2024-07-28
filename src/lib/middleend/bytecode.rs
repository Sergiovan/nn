use std::collections::VecDeque;

use crate::{
	ivec,
	util::indexed_vector::{IndexedVec, IndexedVecIndex},
};

use super::prepare::{FunctionId, ScopeId};

pub type BytecodeId = IndexedVecIndex<Bytecode>;
pub struct RelBytecodeId(i32);

impl RelBytecodeId {
	pub fn new(source: BytecodeId, target: BytecodeId) -> RelBytecodeId {
		RelBytecodeId(target.0 as i32 - source.0 as i32)
	}

	// TODO Overflows and such
	pub fn into_bytecode(self, source: BytecodeId) -> BytecodeId {
		((source.0 as i32 + self.0) as u32).into()
	}
}

pub struct Bytecode {
	pub tag: BytecodeData,
	pub typ: BytecodeType,
}

impl Bytecode {
	pub fn new(data: BytecodeData) -> Bytecode {
		Bytecode {
			tag: data,
			typ: BytecodeType::Never,
		}
	}
}

#[macro_export]
macro_rules! bytecode {
  ([$($type:tt)*] $($data:tt)*) => {
    Bytecode {
      tag: $crate::middleend::bytecode::BytecodeData::$($data)*,
      typ: $crate::middleend::bytecode::BytecodeType::$($type)*,
    }
  };
	($($data:tt)*) => {
		Bytecode {
			tag: $crate::middleend::bytecode::BytecodeData::$($data)*,
			typ: $crate::middleend::bytecode::BytecodeType::Never,
		}
	};
}

pub(crate) use bytecode;

pub enum BytecodeData {
	Error(String),
	Reserved,

	JumpScope(ScopeId),
	JumpRel(RelBytecodeId),
	Jump(BytecodeId),

	FunStart(FunctionId),
	FunEnd(FunctionId),

	ScopeStart(ScopeId),
	ScopeEnd(ScopeId),
}

type TypeId = u32;

pub enum BytecodeType {
	U64,
	Type,
	_Fun(TypeId),

	NoReturn,
	Never,
}
