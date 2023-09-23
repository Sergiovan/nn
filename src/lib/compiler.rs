use crate::frontend::module;

use crate::util::indexed_vector::{IndexedVec, ivec, IndexedVecIndex};

pub struct Compiler<'a> {
	pub modules: IndexedVec<module::Module<'a, 'a>>
}

impl<'a> Compiler<'a> {
	pub fn new() -> Compiler<'a> {
		Compiler {
			modules: ivec![]
		}
	}

	pub fn module_from_string(&mut self, data: String) -> module::ModuleId {
		let id = IndexedVecIndex::from(self.modules.len());
		
		self.modules.push(module::Module::new(id, data));

		id
	}
}