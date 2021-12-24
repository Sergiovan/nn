mod lexer;

fn main() {
    println!("Starting lexer!");
    let mut lexer = lexer::Lexer::from_file("examples/better_c.nn").unwrap();
    let toks = lexer.tokenize();
    println!("{}", toks.recreate());
    // for tok in toks.tokens {
    //     println!("{:?}", tok);
    // }
}
