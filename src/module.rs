use std::cmp::max;

use crate::lexer::{Span, Token, Lexer};
use crate::compiler::Compiler;
use crate::parser::{Ast, Parser, AstType, ast_id};

pub type module_id = usize;

pub struct Module {
    pub id: module_id,
    pub source: String,
    pub tokens: Vec<Token>,
    pub ast_nodes: Vec<Ast>
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

    pub fn parse(&mut self) {
        let mut parser = Parser::new(self);
        parser.parse();
        let (asts, errors) = parser.get_results();
        self.ast_nodes = asts;
        // self.errors = errors;
    }

    pub fn print_token_table(&self, compiler: &Compiler) {
        const TOKEN_TYPE_SIZE: usize = 16;

        let tokens = &self.tokens;
        let idx_size = max(tokens.len().to_string().len(), 3);
        let span_size = self.source.len().to_string().len() * 2 + 1;

        let hdr = format!("{: <idx_width$} | {: <ttype_width$} | {: ^span_width$} | {} ", "IDX", "TOKEN TYPE", "SPAN", "VALUE", idx_width = idx_size, ttype_width = TOKEN_TYPE_SIZE, span_width = span_size);
        println!("{}", hdr);
        println!("{}", "-".repeat(hdr.len()));
        for (i, t) in tokens.iter().enumerate() {
            let token_type_string = format!("{:?}", t.ttype);
            let span_string = format!("{}:{}", t.span.start, t.span.end);
            println!("{: <idx_width$} | {: <ttype_width$} | {: ^span_width$} | {}", i, token_type_string, span_string, t.span.to_string(compiler).replace("\n", ""), idx_width = idx_size, ttype_width = TOKEN_TYPE_SIZE, span_width = span_size);
        }
    }

    pub fn print_ast_helper(&self, c: &Compiler, ast: crate::parser::ast_id, side: String, padding: usize) -> String {
        use AstType as AT;

        let ast_nodes = &self.ast_nodes;
        let node = &ast_nodes[ast];

        let mut res: String = "".to_string(); 

        let add = |txt| format!("{: <padding$}: {}{}", node.id, side, txt, padding = padding);

        match &node.atype {
            AT::Program(id) => {
                res += &add("Program\n");
                res += &self.print_ast_helper(c, *id, "| ".to_string(), padding);
            },
            AT::ErrorAst => {
                res += &add("ErrorAst\n");
            },
            AT::Iden(i) => {
                res += &add(&format!("Iden: {}\n", i));
            },
            AT::Block(elems) => {
                res += &add(&format!("Block ({} elements)\n", elems.len()));
                for id in elems.iter().take(elems.len() - 1) {
                    res += &self.print_ast_helper(c, *id, side.clone() + "| ", padding);
                }
                if elems.len() > 0 {
                    res += &self.print_ast_helper(c, *elems.last().unwrap(), side.clone() + "| ", padding);
                }
            },
            AT::FunctionDefinition { ftype, body } => {
                let s = format!("Function definition\n");
                res += &add(&s);
                let s = format!("  Type\n");
                res += &add(&s);
                res += &self.print_ast_helper(c, *ftype, side.clone() + "  | ", padding);
                let s = format!("  Body\n");
                res += &add(&s);
                res += &self.print_ast_helper(c, *body, side.clone() + "  | ", padding);
            },
            AT::NumberLiteral(i) => {
                res += &add(&format!("Integer: {}\n", *i));
            },
            AT::FunctionType { name, constant_params, params } => {
                let s = format!("Function type\n");
                res += &add(&s);
                let s = format!("  Name\n");
                res += &add(&s);
                res += &self.print_ast_helper(c, *name, side.clone() + "    ", padding);
                // let s = format!("Constant parameters\n");
                // res += &add(&s);
                // res += &self.print_ast_helper(c, *constant_params, side.clone() + "| ", padding);
                // let s = format!("Runtime parameters\n");
                // res += &add(&s);
                // res += &self.print_ast_helper(c, *params, side.clone() + "| ", padding);
            },
            AT::Return(id) => {
                res += &add(&format!("Return\n"));
                res += &self.print_ast_helper(c, *id, side.clone() + "| ", padding)
            },
            AT::Add { left, right } => {
                res += &add(&format!("Add\n"));
                res += &self.print_ast_helper(c, *left, side.clone() + "| ", padding);
                res += &self.print_ast_helper(c, *right, side.clone() + "| ", padding);
            },
            AT::FunctionCall { function, params } => {
                let s = format!("Function call\n");
                res += &add(&s);
                let s = format!("  Function\n");
                res += &add(&s);
                res += &self.print_ast_helper(c, *function, side.clone() + "  | ", padding);
                // let s = format!("Arguments\n");
                // res += &add(&s);
                // res += &self.print_ast_helper(c, *params, side.clone() + "| ", padding);
            },
            AT::FunctionParams() => {

            },
            AT::FunctionParam() => {

            },
        }
        res
    }

    pub fn print_ast(&self, compiler: &Compiler) {
        let padding = self.ast_nodes.len().to_string().len() + 1;
        print!("{}", self.print_ast_helper(compiler, self.ast_nodes.len() - 1, "".to_owned(), padding));
    }
}