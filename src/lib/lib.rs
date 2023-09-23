mod compiler;
mod util;
mod frontend;

use std::fs;
use std::path::Path;

pub fn compile(path: &Path) {
	let mut c = compiler::Compiler::new();

	let module = c.module_from_string(fs::read_to_string(path).unwrap().to_owned());
	let module = &mut c.modules[module];

	module.lex();

	module.print_token_table();

	println!("\n\n");

	module.parse();

	module.print_ast();
}