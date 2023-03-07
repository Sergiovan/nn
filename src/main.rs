mod compiler;
mod module;
mod lexer;

use std::fs;

fn main() {
    let mut c = compiler::Compiler::new();

    let module = c.module_from_string(fs::read_to_string("examples/sixty_nine.nn").unwrap().to_owned());
    c.modules[module].lex();

    c.modules[module].print_token_table(&c);
}