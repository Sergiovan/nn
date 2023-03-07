use crate::module;

pub struct Compiler {
    pub modules: Vec<module::Module>
}

impl Compiler {
    pub fn new() -> Compiler {
        Compiler {
            modules: vec![]
        }
    }

    pub fn module_from_string(&mut self, data: String) -> module::module_id {
        let id = self.modules.len();
        
        self.modules.push(module::Module {
            id: id,
            source: data,
            tokens: vec![]
        });

        id
    }
}