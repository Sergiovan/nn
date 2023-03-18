mod compiler;
mod module;
mod lexer;
mod parser;

mod util;

use std::fs;

fn main() {
    let mut c = compiler::Compiler::new();

    let module = c.module_from_string(fs::read_to_string("examples/sixty_nine.nn").unwrap().to_owned());
    let module = &mut c.modules[module];

    module.lex();

    module.print_token_table();

    module.parse();

    module.print_ast();
}