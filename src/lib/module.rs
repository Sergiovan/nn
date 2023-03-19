use std::cmp::max;

use crate::lexer::{Token, Lexer};
use crate::parser::{Ast, AstId, Parser, AstType};

use crate::util::IndexedVector::{IndexedVector, IVec, ivec};

#[derive(Copy, Clone, Debug)]
pub struct Span<'a> {
    pub text: &'a str,
    pub module_idx: ModuleId,
    pub start: u32,
    pub line: u32,
    pub col: u32
}

impl<'a> ToString for Span<'a> {
    fn to_string(&self) -> String {
        self.text.to_owned()
    }
}

pub type ModuleId = <IVec<Module<'static, 'static>> as IndexedVector>::Index;

pub struct Module<'a, 'b: 'a> {
    pub id: ModuleId,
    source: &'b mut String,
    tokens: Vec<Token<'a>>,
    ast_nodes: IVec<Ast<'a>>,
}

impl<'a, 'b> Module<'a, 'b> {
    pub fn new(id: ModuleId, source: String) -> Module<'a, 'b> {
        // We fenaggle a heap-string here, to do some lifetime hacks later
        // Listen, this is awful but I don't have enough knowledge to figure
        // out the proper way to do it, and google is being unhelpful.
        let mut _box = Box::new(source);
        let ptr = &mut *Box::leak(_box) as *mut String;

        Module {
            id,
            source: unsafe { &mut *ptr },
            tokens: vec![],
            ast_nodes: ivec![]
        }
    }

    pub fn new_span(&'a self, start: u32, end: u32, line: u32, col: u32) -> Span<'b> {
        Span {
            text: &self.get_source()[(start as usize)..(end as usize)],
            module_idx: self.id,
            start: start,
            line: line,
            col: col
        }
    }

    pub fn get_source(&'a self) -> &'b String {
        unsafe { &*(self.source as *const String) }
    }

    pub fn lex(&mut self) {
        let tokens = {
            let lexer = Lexer::new(&self);
            lexer.lex()
        };
        self.tokens = tokens;
    }

    pub fn parse(&mut self) {
        let (asts, _errors) = {
            // Safe because asts only depend on source (via Span) and are stored in self
            // Once again, I don't understand what's happening when I do a simple
            // let toks = self.tokens;
            // but it complains about borrows outliving the scope of the function
            // presumably because some Span is not being done properly
            let toks = unsafe { &*(&self.tokens as *const Vec<Token<'_>>) };
            let iter = toks.iter();
            let mut parser = Parser::new(&self, iter.peekable());
            parser.parse();
            parser.get_results()
        };
        self.ast_nodes = asts;
        // self.errors = errors;
    }

    pub fn print_token_table(&self) {
        const TOKEN_TYPE_SIZE: usize = 16;

        let tokens = &self.tokens;
        let idx_size = max(tokens.len().to_string().len(), 3);
        let span_size = self.get_source().len().to_string().len() * 2 + 1;

        let hdr = format!("{: <idx_width$} | {: <ttype_width$} | {: ^span_width$} | {} ", 
                          "IDX", "TOKEN TYPE", "SPAN", "VALUE", 
                          idx_width = idx_size, ttype_width = TOKEN_TYPE_SIZE, span_width = span_size);
        println!("{}", hdr);
        println!("{}", "-".repeat(hdr.len()));
        for (i, t) in tokens.iter().enumerate() {
            let token_type_string = format!("{:?}", t.ttype);
            let span_string = format!("{}:{}", t.span.start, t.span.text.len());
            println!("{: <idx_width$} | {: <ttype_width$} | {: ^span_width$} | {}", 
                     i, token_type_string, span_string, t.span.to_string().replace("\n", ""), 
                     idx_width = idx_size, ttype_width = TOKEN_TYPE_SIZE, span_width = span_size);
        }
    }

    pub fn print_ast_helper(&self, ast: crate::parser::AstId, side: String, padding: usize) -> String {
        use AstType as AT;

        const PIPE: char = '┃';
        const FORK: char = '┣';
        const LAST: char = '┗';
        const NONE: char = ' ';

        fn convert_side(previous: &str) -> String {
            let mut s = String::with_capacity(previous.len() + 2);

            for c in previous.chars() {
                let new_char: char = match c {
                    PIPE => PIPE,
                    FORK => PIPE,
                    LAST => NONE,
                    NONE => NONE,
                    _ => c
                };
                s.push(new_char);
            }

            s
        }

        let ast_nodes = &self.ast_nodes;
        let node = &ast_nodes[ast];

        let mut res: String = "".to_string(); 

        let add_header = |txt| format!("{: >padding$}: {}{}", node.id.to_string(), side, txt, padding = padding);
        let new_side = convert_side(&side);
        let add_child = |txt| format!("{: >padding$}: {}{}", node.id.to_string(), new_side, txt, padding = padding);

        match &node.atype {
            AT::Program(id) => {
                res += &add_header("Program\n");
                res += &self.print_ast_helper(*id, format!("{} ", LAST), padding);
            },
            AT::ErrorAst => {
                res += &add_header("ErrorAst\n");
            },
            AT::Iden => {
                res += &add_header(&format!("Iden: {}\n", node.span.to_string()));
            },
            AT::Block(elems) => {
                res += &add_header(&format!("Block ({} elements)\n", elems.len()));
                for id in elems.iter().take(elems.len() - 1) {
                    res += &self.print_ast_helper(*id, new_side.clone() + &format!("{} ", FORK), padding);
                }
                if elems.len() > 0 {
                    res += &self.print_ast_helper(*elems.last().unwrap(), new_side.clone() + &format!("{} ", LAST), padding);
                }
            },
            AT::FunctionDefinition { ftype, body } => {
                let s = format!("Function definition\n");
                res += &add_header(&s);
                let s = format!("{} Type\n", FORK);
                res += &add_child(&s);
                res += &self.print_ast_helper(*ftype, new_side.clone() + &format!("{} {} ", PIPE, FORK), padding);
                let s = format!("{} Body\n", LAST);
                res += &add_child(&s);
                res += &self.print_ast_helper(*body, new_side.clone() + &format!("  {} ", LAST), padding);
            },
            AT::NumberLiteral => {
                res += &add_header(&format!("Integer: {}\n", node.span.to_string()));
            },
            AT::FunctionType { name, .. } => {
                let s = format!("Function type\n");
                res += &add_header(&s);
                let s = format!("{} Name\n", LAST);
                res += &add_child(&s);
                res += &self.print_ast_helper(*name, new_side.clone() + &format!("  {} ", LAST), padding);
                // let s = format!("Constant parameters\n");
                // res += &add(&s);
                // res += &self.print_ast_helper(c, *constant_params, side.clone() + "| ", padding);
                // let s = format!("Runtime parameters\n");
                // res += &add(&s);
                // res += &self.print_ast_helper(c, *params, side.clone() + "| ", padding);
            },
            AT::Return(id) => {
                res += &add_header(&format!("Return\n"));
                res += &self.print_ast_helper(*id, new_side.clone() + &format!("{} ", LAST), padding)
            },
            AT::Add { left, right } => {
                res += &add_header(&format!("Add\n"));
                res += &self.print_ast_helper(*left, new_side.clone() + &format!("{} ", FORK), padding);
                res += &self.print_ast_helper(*right, new_side.clone() + &format!("{} ", LAST), padding);
            },
            AT::FunctionCall { function, .. } => {
                let s = format!("Function call\n");
                res += &add_header(&s);
                let s = format!("{} Function\n", LAST);
                res += &add_child(&s);
                res += &self.print_ast_helper(*function, new_side.clone() + &format!("  {} ", LAST), padding);
                // let s = format!("Arguments\n");
                // res += &add(&s);
                // res += &self.print_ast_helper(c, *params, side.clone() + "| ", padding);
            },
            AT::FunctionParams() => {

            },
            AT::_FunctionParam() => {

            },
        }
        res
    }

    pub fn print_ast(&self) {
        let padding = self.ast_nodes.len().to_string().len() + 1;
        print!("{}", self.print_ast_helper(AstId::from(self.ast_nodes.len() - 1), "".to_owned(), padding));
    }
}

impl<'a, 'b> Drop for Module<'a, 'b> {
    fn drop(&mut self) {
        // Undo hacks from earlier
        unsafe { 
            let _ = Box::from_raw(self.source as *mut String);
        }
    }
}