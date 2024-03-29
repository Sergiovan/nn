use crate::{ivec, util::indexed_vector::IndexedVec};

use self::ir::{Ir, IrId, Type};

use super::{
	module::span::Span,
	parser::ast::{Ast, AstId, AstType},
};

pub mod ir;

#[derive(Debug)]
pub struct IrGeneratorError(String);

impl std::fmt::Display for IrGeneratorError {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		self.0.fmt(f)
	}
}

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

	pub fn generate(&mut self) {
		self.convert((self.asts.len() - 1).into());
	}

	pub fn get_results(self) -> (IndexedVec<Ir>, Vec<IrGeneratorError>) {
		(self.ir, self.errors)
	}

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

	fn convert(&mut self, ast_id: AstId) -> IrId {
		let ast = self.ast(ast_id);
		match ast {
			AstType::Program(block) => self.convert(*block),
			AstType::ErrorAst => {
				self.error(format!("Found error AST @ {}", self.asts[ast_id].span))
			}
			AstType::Iden => self.push(Ir::Iden(self.asts[ast_id].span.to_string())),
			AstType::Symbol => self.push(Ir::Symbol(self.asts[ast_id].span.to_string())),
			AstType::TopBlock(stmts) => self.block(stmts),
			AstType::FunctionBlock(stmts) => self.block(stmts),
			AstType::_Block(stmts) => self.block(stmts),
			AstType::FunctionDefinition { ftype, body } => self.function_definition(*ftype, *body),
			AstType::NumberLiteral => self.number_literal(&self.asts[ast_id].span),
			AstType::FunctionType { .. } => self.error(format!(
				"Found function type where it's not available (yet): {}",
				self.asts[ast_id].span
			)),
			AstType::Return(r) => self.r#return(*r),
			AstType::Add { left, right } => self.add(*left, *right),
			AstType::FunctionCall { function, args } => self.function_call(*function, *args),
			AstType::FunctionParams() => self.empty_block(),
			AstType::_FunctionParam() => {
				self.error(format!("Found invalid AST @ {}", self.asts[ast_id].span))
			}
		}
	}

	fn block(&mut self, stmts: &[AstId]) -> IrId {
		let block_id = self.reserve();
		let block = stmts.iter().map(|s| self.convert(*s));

		self.ir[block_id] = Ir::Block(block.collect());
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
			constant_params: _,
			params,
		} = self.ast(ftype)
		else {
			return self.error(format!(
				"Expected function type, found {:?}",
				self.asts[ftype]
			));
		};

		let AstType::FunctionBlock(..) = self.ast(body) else {
			return self.error(format!(
				"Expected function body, found {:?}",
				self.asts[body]
			));
		};

		let iden = self.convert(*name);
		// constant params
		let params = self.convert(*params); // For now

		let returns = self.block_of(vec![Ir::Type(Type::U64)]);

		let body = self.convert(body);

		self.push(Ir::Deffun {
			iden,
			params,
			returns,
			body,
		})
	}

	fn number_literal(&mut self, span: &Span) -> IrId {
		let n = span.to_string().parse::<u64>();

		let Ok(n) = n else {
			return self.error(format!("Error while parsing {}: {}", span, n.unwrap_err()));
		};

		let typ = self.push(Ir::Type(Type::U64));
		let number = self.push(Ir::Number(n));

		self.push(Ir::Value {
			value: number,
			r#type: typ,
		})
	}

	fn r#return(&mut self, ast_id: AstId) -> IrId {
		// TODO Figure out function nesting
		let value = self.convert(ast_id);

		self.push(Ir::Return {
			value,
			block: u32::MAX.into(),
		})
	}

	fn add(&mut self, left_id: AstId, right_id: AstId) -> IrId {
		let left = self.convert(left_id);
		let right = self.convert(right_id);

		self.push(Ir::Add {
			lhs: left,
			rhs: right,
		})
	}

	fn function_call(&mut self, function_id: AstId, _args_id: AstId) -> IrId {
		let function = self.convert(function_id);
		let args = u32::MAX.into();

		self.push(Ir::Call {
			fun: function,
			args,
		})
	}
}
