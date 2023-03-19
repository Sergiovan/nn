use crate::module;

use crate::util::IndexedVector::{IVec, ivec, VecId};

pub struct Compiler<'a> {
    pub modules: IVec<module::Module<'a, 'a>>
}

impl<'a> Compiler<'a> {
    pub fn new() -> Compiler<'a> {
        Compiler {
            modules: ivec![]
        }
    }

    pub fn module_from_string(&mut self, data: String) -> module::ModuleId {
        let id = VecId::from(self.modules.len());
        
        self.modules.push(module::Module::new(VecId::from(id), data));

        id
    }
}