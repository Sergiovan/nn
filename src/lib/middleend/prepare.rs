use std::collections::VecDeque;

use crate::{
	frontend::ir_gen::ir::{Ir, IrId},
	ivec,
	util::indexed_vector::{IndexedVec, IndexedVecIndex},
};

use super::bytecode::{bytecode, Bytecode, BytecodeId, RelBytecodeId};

pub enum SymbolType {
	Universal,
}

pub enum SymbolLocation {
	Global(BytecodeId),
	EndOfScope(RelBytecodeId),
	Undefined,
}

type SymbolId = IndexedVecIndex<Symbol>;

pub struct Symbol {
	name: String,
	typ: SymbolType,
	loc: SymbolLocation,
}

impl Symbol {
	pub fn new(name: String, typ: SymbolType, location: SymbolLocation) -> Symbol {
		Symbol {
			name,
			typ,
			loc: location,
		}
	}

	pub fn fix_location(&mut self, location: SymbolLocation) {
		self.loc = location;
	}
}

pub type ScopeId = IndexedVecIndex<Scope>;
pub struct Scope {
	id: ScopeId,
	parent: Option<ScopeId>,

	symbols: Vec<SymbolId>,

	exit_code: Option<VecDeque<Bytecode>>,
}

impl Scope {
	pub fn new(id: ScopeId, parent: Option<ScopeId>) -> Scope {
		Scope {
			id,
			parent,

			symbols: vec![],
			exit_code: None,
		}
	}

	pub fn add_symbol(&mut self, sym: SymbolId) {
		self.symbols.push(sym);
	}

	pub fn id(&self) -> ScopeId {
		self.id
	}
}

pub type FunctionId = IndexedVecIndex<Function>;

pub struct Function {
	id: FunctionId,
	top_scope: ScopeId,
	start: BytecodeId,
	end: BytecodeId,
}

impl Function {
	pub fn top_scope(&self) -> ScopeId {
		self.top_scope
	}
}

pub struct Prepare<'a> {
	functions: IndexedVec<Function>,
	scopes: IndexedVec<Scope>,
	code: IndexedVec<Bytecode>,
	symbols: IndexedVec<Symbol>,

	irs: &'a IndexedVec<Ir>,

	function_stack: Vec<FunctionId>,
	scope_stack: Vec<ScopeId>,
}

impl<'a> Prepare<'a> {
	pub fn new(irs: &IndexedVec<Ir>) -> Prepare {
		Prepare {
			functions: ivec![],
			scopes: ivec![],
			code: ivec![],
			symbols: ivec![],

			irs,
			function_stack: Default::default(),
			scope_stack: Default::default(),
		}
	}

	pub fn prepare(&mut self) {
		let _top = self.enter_scope(); // Top scope

		self.top_block();
	}

	fn top_block(&mut self) {
		if let Some(Ir::Block(elems)) = self.irs.first() {
			// First pass to put things in their place
			for irid in elems {
				let ir = &self.irs[*irid];
				match ir {
					Ir::Deffun { iden, .. } => {
						let s = self.str_from_iden(*iden);
						let sym = self.new_symbol(Symbol::new(
							s,
							SymbolType::Universal,
							SymbolLocation::Undefined,
						));
						self.top_scope().add_symbol(sym);
					}
					Ir::Reserved => (),
					_ => (),
				}
			}

			// Second pass to actually do stuff
			for irid in elems {
				let ir = &self.irs[*irid];
				match ir {
					Ir::Deffun { .. } => self.deffun(*irid),
					Ir::Reserved => {
						self.add(bytecode!(Reserved));
					}
					Ir::Error(s) => {
						self.add(bytecode!(Error(s.clone())));
					}
					_ => self.error("Invalid bytecode at top level"),
				}
			}
		} else {
			self.error_bytecode(&format!("First IR was not block: {:?}", self.irs.first()));
		}
	}

	fn deffun(&mut self, ir: IrId) {
		let Ir::Deffun {
			iden,
			params,
			returns,
			body,
		} = self.irs[ir]
		else {
			unreachable!("Function was not checked before entering: {}", self.irs[ir]);
		};

		let funguard = self.enter_function();
		let scope = self.top_scope();

		// TODO Add params to symbol table
		// TODO Add returns? to symbol table

		self.stmts(body);
	}

	fn stmts(&mut self, block_ir: IrId) {
		let Ir::Block(stmts) = &self.irs[block_ir] else {
			unreachable!("Block was no block");
		};
	}

	fn str_from_iden(&mut self, ir: IrId) -> String {
		if let Ir::Iden(s) = &self.irs[ir] {
			s.clone()
		} else {
			self.error(&format!("IR was not iden: {}", self.irs[ir]));

			"RandomString".to_string()
		}
	}

	#[must_use]
	fn enter_function(&mut self) -> FunCtxGuard<'a> {
		let id = self.functions.len().into();

		let start_code = self.add(bytecode!(FunStart(id)));

		let scope_id = self.enter_scope().manual();

		self.functions.push(Function {
			id,
			top_scope: scope_id,
			start: start_code,
			end: 0usize.into(),
		});

		self.function_stack.push(id);

		FunCtxGuard::new(self, id)
	}

	fn exit_function(&mut self) -> FunctionId {
		let id = self.function_stack.pop().unwrap();

		self.exit_scope(); // Function scope

		self.add(bytecode!(FunEnd(id)));

		id
	}

	#[must_use]
	fn enter_scope(&mut self) -> ScopeCtxGuard<'a> {
		let id = self.scopes.len().into();

		self.add(bytecode!(ScopeStart(id)));

		self.scopes.push(Scope::new(
			id,
			if self.scope_stack.is_empty() {
				None
			} else {
				unsafe { Some(*self.scope_stack.last().unwrap_unchecked()) }
			},
		));

		self.scope_stack.push(id);

		ScopeCtxGuard::new(self, id)
	}

	fn exit_scope(&mut self) -> ScopeId {
		let id = self.scope_stack.pop().unwrap();

		self.add(bytecode!(ScopeEnd(id)));

		id
	}

	fn top_scope(&mut self) -> &mut Scope {
		&mut self.scopes[*self.scope_stack.last().unwrap()]
	}

	fn add(&mut self, bytecode: Bytecode) -> BytecodeId {
		let id = self.code.len().into();
		self.code.push(bytecode);
		id
	}

	fn new_symbol(&mut self, symbol: Symbol) -> SymbolId {
		let id = self.symbols.len().into();
		self.symbols.push(symbol);
		id
	}

	fn find_symbol_in_scope(&self, name: &str, scope: ScopeId) -> Option<SymbolId> {
		let scope = &self.scopes[scope];
		for sym_id in &scope.symbols {
			let sym = &self.symbols[*sym_id];
			if sym.name == name {
				return Some(*sym_id);
			}
		}
		None
	}

	fn find_symbol_full(&mut self, name: &str) -> Option<SymbolId> {
		let scope = self.top_scope().id();
		self.find_symbol_full_helper(name, scope)
	}

	fn find_symbol_full_helper(&mut self, name: &str, scope: ScopeId) -> Option<SymbolId> {
		{
			let scope = &self.scopes[scope];
			{
				let sym = self.find_symbol_in_scope(name, scope.id());
				if sym.is_some() {
					return sym;
				}
			}
			if let Some(parent) = &scope.parent {
				self.find_symbol_full_helper(name, *parent)
			} else {
				None
			}
		}
	}

	fn error(&mut self, msg: &str) {
		panic!("{msg}");
	}

	fn error_bytecode(&mut self, msg: &str) -> BytecodeId {
		panic!("{msg}"); // For now
	}
}

/// SAFETY: Neither context guard escapes the methods of the Prepare from
/// which they came from, thus it always exists at dereference point.
/// The elements they guard are copy and must not be referenced.
struct FunCtxGuard<'a> {
	prepare: *mut Prepare<'a>,
	pub id: FunctionId,
	pop: bool,
}

impl<'a> FunCtxGuard<'a> {
	pub fn new(prepare: &mut Prepare<'a>, id: FunctionId) -> FunCtxGuard<'a> {
		FunCtxGuard {
			prepare,
			id,
			pop: true,
		}
	}

	pub fn manual(mut self) -> FunctionId {
		self.pop = false;
		self.id
	}
}

impl<'a> Drop for FunCtxGuard<'a> {
	fn drop(&mut self) {
		if self.pop {
			unsafe { &mut *self.prepare }.function_stack.pop();
		}
	}
}

struct ScopeCtxGuard<'a> {
	prepare: *mut Prepare<'a>,
	pub id: ScopeId,
	pop: bool,
}

impl<'a> ScopeCtxGuard<'a> {
	pub fn new(prepare: &mut Prepare<'a>, id: ScopeId) -> ScopeCtxGuard<'a> {
		ScopeCtxGuard {
			prepare,
			id,
			pop: true,
		}
	}

	pub fn manual(mut self) -> ScopeId {
		self.pop = false;
		self.id
	}
}

impl<'a> Drop for ScopeCtxGuard<'a> {
	fn drop(&mut self) {
		if self.pop {
			unsafe { &mut *self.prepare }.scope_stack.pop();
		}
	}
}
