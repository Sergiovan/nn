use std::collections::VecDeque;

use crate::{
	ivec,
	util::indexed_vector::{IndexedVec, IndexedVector},
};

use self::ir::{Ir, IrId, Type};

use super::parser::ast::{Ast, AstId, AstType};

pub mod ir;

#[derive(Debug)]
pub struct IrGeneratorError(String);

pub struct IrGenerator<'a> {
	asts: &'a IndexedVec<Ast>,

	ir: IndexedVec<Ir>,
	errors: Vec<IrGeneratorError>,
}

impl<'a> IrGenerator<'a> {
	pub fn new(asts: &IndexedVec<Ast>) -> IrGenerator {
		IrGenerator {
			asts,
			ir: ivec![],
			errors: vec![],
		}
	}

	pub fn generate(&mut self) {}

	pub fn get_results(self) {}

	fn ast(&self, ast: AstId) -> &'a AstType {
		&self.asts[ast].atype
	}

	fn error<T: Into<String>>(&mut self, err: T) -> IrId {
		let err = err.into();
		self.errors.push(IrGeneratorError(err.clone()));
		self.push(Ir::Error(err))
	}

	fn push(&mut self, ir: Ir) -> IrId {
		let res = self.ir.len().into();
		self.ir.push(ir);
		res
	}

	fn reserve(&mut self) -> IrId {
		let res = self.ir.len().into();
		self.ir.push(Ir::Reserved);
		res
	}

	fn program(&mut self, ast: AstId) {
		let AstType::Program(block) = self.ast(ast) else {
			self.error(format!("Expected Program, found: {:?}", self.asts[ast]));
			return;
		};
		_ = self.block(*block, Self::top_level_block);
	}

	fn top_level_block(&mut self, ast: &AstType) -> IrId {
		match ast {
			AstType::FunctionDefinition { ftype, body } => self.function_definition(*ftype, *body),
			_ => self.error(format!("Expected FunctionDefinition, got {} instead", ast)),
		}
	}

	fn function_body_block(&mut self, ast: &AstType) -> IrId {
		match ast {
			AstType::Add { left, right } => todo!(),
			AstType::Return(val) => todo!(),
			AstType::FunctionCall { function, params } => todo!(),
			_ => self.error(format!("Invalid ast found in function body: {}", ast)),
		}
	}

	fn block(&mut self, ast: AstId, matcher: fn(&mut Self, &AstType) -> IrId) -> IrId {
		let block_id = self.reserve();
		let mut block = vec![];
		let AstType::Block(elems) = self.ast(ast) else {
			return self.error(format!("Expected block, found: {:?}", self.asts[ast]));
		};

		for elem in elems {
			let elem = self.ast(*elem);
			block.push(matcher(self, elem));
		}

		self.ir[block_id] = Ir::Block(block);
		block_id
	}

	fn empty_block(&mut self) -> IrId {
		self.push(Ir::Block(vec![]))
	}

	fn block_of(&mut self, irs: Vec<Ir>) -> IrId {
		let block_id = self.reserve();
		let block = irs.into_iter().map(|ir| self.push(ir));

		self.ir[block_id] = Ir::Block(block.collect());
		block_id
	}

	fn function_definition(&mut self, ftype: AstId, body: AstId) -> IrId {
		let AstType::FunctionType {
			name,
			constant_params,
			params,
		} = self.ast(ftype)
		else {
			return self.error(format!(
				"Expected function type, found {:?}",
				self.asts[ftype]
			));
		};

		let AstType::Block(elems) = self.ast(body) else {
			return self.error(format!(
				"Expected function body, found {:?}",
				self.asts[body]
			));
		};

		let iden = self.iden(*name);
		// constant params
		let params = self.empty_block(); // For now

		let returns = self.block_of(vec![Ir::Type(Type::U64)]);

		let body = self.block(body, Self::function_body_block);

		self.push(Ir::Deffun {
			iden,
			params,
			returns,
			body,
		})
	}

	fn iden(&mut self, iden: AstId) -> IrId {
		let AstType::Iden = self.ast(iden) else {
			return self.error(format!("Expected iden, found {:?}", self.asts[iden]));
		};

		let iden_span = self.asts[iden].span.to_string();

		return self.push(Ir::Iden(iden_span));
	}

	fn add(&mut self, left: AstId, right: AstId) -> IrId {
		todo!()
	}
}
