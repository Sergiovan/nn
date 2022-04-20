mod lexer;
mod parser;
mod ast;

fn main() {
    println!("Starting lexer!");
    let mut lexer = lexer::Lexer::from_file("examples/better_c.nn").unwrap();
    let toks = lexer.tokenize();
    // for tok in &toks.tokens {
    //     println!("{:?}", tok);
    // }
    // println!("\n{}", toks.recreate());
    let mut parser = parser::Parser::new(&toks);
    let res = parser.parse_program();
    println!("{:?}", res);
}
