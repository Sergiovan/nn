use std::cmp::max;

use crate::lexer::{Span, Token, Lexer};
use crate::compiler::Compiler;

pub type module_id = usize;

pub struct Module {
    pub id: module_id,
    pub source: String,
    pub tokens: Vec<Token>
}

impl Module {
    pub fn new_span(&self, start: usize, end: usize) -> Span {
        Span {
            module_idx: self.id,
            start: start,
            end: end
        }
    }

    pub fn text_from_span(&self, span: &Span) -> String {
        self.source.chars().skip(span.start).take((span.end - span.start) + 1).collect()
    }

    pub fn lex(&mut self) {
        let mut lexer = Lexer::new(self);
        self.tokens = lexer.lex();
    }

    pub fn print_token_table(&self, compiler: &Compiler) {
        let tokens = &self.tokens;
        let token_type_size = 16;
        let idx_size = max(tokens.len().to_string().len(), 3);

        let hdr = format!("{: <idx_width$} | {: <ttype_width$} | {} ", "IDX", "TOKEN TYPE", "VALUE", idx_width = idx_size, ttype_width = token_type_size);
        println!("{}", hdr);
        println!("{}", "-".repeat(hdr.len()));
        for (i, t) in tokens.iter().enumerate() {
            let token_type_string = format!("{:?}", t.ttype);
            println!("{: <idx_width$} | {: <ttype_width$} | {}", i, token_type_string, t.span.to_string(compiler).replace("\n", ""), idx_width = idx_size, ttype_width = token_type_size);
        }
    }
}