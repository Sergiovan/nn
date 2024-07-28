#![feature(mapped_lock_guards)]

mod compiler;
mod frontend;
mod middleend;
mod util;

use std::fs;
use std::path::Path;

pub fn compile(path: &Path) {
	let mut c = compiler::Compiler::new();

	let module = c.module_from_string(fs::read_to_string(path).unwrap().to_owned());
	let module = &mut c.modules[module];

	module.lex();

	module.print_token_table();

	println!();

	module.parse();

	println!();

	module.print_ast();

	println!();

	module.gen_ir();

	// println!();

	// module.enumerate_ir();

	println!();

	module.print_ir();

	println!();

	module.prepare();
}
