pub mod span;

use self::span::Span;

use super::ir_gen::ir::{Ir, IrId};
use super::ir_gen::IrGenerator;
use super::lexer::{token::Token, Lexer};
use super::parser::{
	ast::{Ast, AstId, AstType},
	Parser,
};

use crate::util::indexed_vector::{ivec, IndexedVec, IndexedVector};

use std::cell::{Ref, RefCell};
use std::cmp::max;

pub type ModuleId = <IndexedVec<Module> as IndexedVector>::Index;

#[derive(Debug)]
pub struct Module {
	pub id: ModuleId,
	source_ptr: *mut String,
	source: &'static str,
	tokens: RefCell<Vec<Token>>,
	eof_token: Token,
	ast_nodes: RefCell<IndexedVec<Ast>>,
	ir: RefCell<IndexedVec<Ir>>,
}

impl Module {
	pub fn new(id: ModuleId, source: String) -> Module {
		use super::lexer::token::TokenType;

		let source_len = source.len();
		let last_line = source.lines().count();
		let last_col = source.lines().last().map_or(0, |l| l.len());

		// We fenaggle a heap-string here, to do some lifetime hacks later
		// Listen, this is awful but I don't have enough knowledge to figure
		// out the proper way to do it, and google is being unhelpful.
		let mut _box = Box::new(source);
		let ptr = &mut *Box::leak(_box) as *mut String;

		let source: &'static str = unsafe { &*ptr };

		Module {
			id,
			source_ptr: ptr,
			source,
			tokens: RefCell::new(vec![]),
			eof_token: Token {
				ttype: TokenType::Eof,
				span: Span {
					text: "",
					module_idx: id,
					start: source_len as u32 - 1_u32,
					line: last_line as u32,
					col: last_col as u32,
				},
			},
			ast_nodes: RefCell::new(ivec![]),
			ir: RefCell::new(ivec![]),
		}
	}

	// Spans depend on the lifetime of modules,
	// but since modules live forever this is not an issue
	pub fn new_span(&self, start: u32, end: u32, line: u32, col: u32) -> Span {
		Span {
			text: &self.source()[(start as usize)..(end as usize)],
			module_idx: self.id,
			start,
			line,
			col,
		}
	}

	pub fn source(&self) -> &'static str {
		self.source
	}

	pub fn tokens(&self) -> Ref<Vec<Token>> {
		self.tokens.borrow()
	}

	pub fn eof_token(&self) -> &Token {
		&self.eof_token
	}

	pub fn ast_nodes(&self) -> Ref<IndexedVec<Ast>> {
		self.ast_nodes.borrow()
	}

	pub fn ir(&self) -> Ref<IndexedVec<Ir>> {
		self.ir.borrow()
	}

	pub fn lex(&self) {
		let tokens = {
			let lexer = Lexer::new(self);
			lexer.lex()
		};
		*self.tokens.borrow_mut() = tokens;
	}

	pub fn parse(&self) {
		let (asts, errors) = {
			let toks = self.tokens.borrow();
			let iter = toks.iter();
			let mut parser = Parser::new(self, iter.peekable());
			parser.parse();
			parser.get_results()
		};
		*self.ast_nodes.borrow_mut() = asts;
		for error in errors.into_iter() {
			println!("{:?}", error);
		}
	}

	pub fn gen_ir(&self) {
		let (ir, errors) = {
			let ast = self.ast_nodes();

			let mut ir_generator = IrGenerator::new(&ast);
			ir_generator.generate();
			ir_generator.get_results()
		};

		*self.ir.borrow_mut() = ir;
		for error in errors.into_iter() {
			println!("{:?}", error);
		}
	}

	pub fn print_token_table(&self) {
		const TOKEN_TYPE_SIZE: usize = 16;

		let tokens = self.tokens();
		let idx_size = max(tokens.len().to_string().len(), 3);
		let span_size = self.source().len().to_string().len() * 2 + 1;

		let hdr = format!(
			"{: <idx_width$} | {: <ttype_width$} | {: ^span_width$} | {} ",
			"IDX",
			"TOKEN TYPE",
			"SPAN",
			"VALUE",
			idx_width = idx_size,
			ttype_width = TOKEN_TYPE_SIZE,
			span_width = span_size
		);
		println!("{}", hdr);
		println!("{}", "-".repeat(hdr.len()));
		for (i, t) in tokens.iter().enumerate() {
			let token_type_string = format!("{:?}", t.ttype);
			let span_string = format!("{}:{}", t.span.start, t.span.text.len());
			println!(
				"{: <idx_width$} | {: <ttype_width$} | {: ^span_width$} | {}",
				i,
				token_type_string,
				span_string,
				t.span.to_string().replace('\n', ""),
				idx_width = idx_size,
				ttype_width = TOKEN_TYPE_SIZE,
				span_width = span_size
			);
		}
	}

	pub fn print_ast_helper(&self, ast: AstId, side: String, padding: usize) -> String {
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
					_ => c,
				};
				s.push(new_char);
			}

			s
		}

		let ast_nodes = self.ast_nodes.borrow();
		let node = &ast_nodes[ast];

		let mut res: String = "".to_string();

		let add_header = |txt| {
			format!(
				"${: <padding$}: {}{}",
				node.id.to_string(),
				side,
				txt,
				padding = padding
			)
		};
		let new_side = convert_side(&side);
		let add_child = |txt| {
			format!(
				"${: <padding$}: {}{}",
				node.id.to_string(),
				new_side,
				txt,
				padding = padding
			)
		};

		match &node.atype {
			AT::Program(id) => {
				res += &add_header("Program\n");
				res += &self.print_ast_helper(*id, format!("{} ", LAST), padding);
			}
			AT::ErrorAst => {
				res += &add_header("ErrorAst\n");
			}
			AT::Iden => {
				res += &add_header(&format!("Iden: {}\n", node.span.to_string()));
			}
			AT::Symbol => {
				res += &add_header(&format!("Symbol: {}\n", node.span.to_string()));
			}
			AT::TopBlock(elems) | AT::FunctionBlock(elems) | AT::_Block(elems) => {
				let name = if let AT::TopBlock(..) = &node.atype {
					"Top level block"
				} else if let AT::FunctionBlock(..) = &node.atype {
					"Function body"
				} else {
					"Block"
				};
				res += &add_header(&format!("{} ({} elements)\n", name, elems.len()));
				for id in elems.iter().take(elems.len() - 1) {
					res += &self.print_ast_helper(
						*id,
						new_side.clone() + &format!("{} ", FORK),
						padding,
					);
				}
				if !elems.is_empty() {
					res += &self.print_ast_helper(
						*elems.last().unwrap(),
						new_side.clone() + &format!("{} ", LAST),
						padding,
					);
				}
			}
			AT::FunctionDefinition { ftype, body } => {
				let s = "Function definition\n".to_string();
				res += &add_header(&s);
				let s = format!("{} Type\n", FORK);
				res += &add_child(&s);
				res += &self.print_ast_helper(
					*ftype,
					new_side.clone() + &format!("{} {} ", PIPE, FORK),
					padding,
				);
				let s = format!("{} Body\n", LAST);
				res += &add_child(&s);
				res += &self.print_ast_helper(
					*body,
					new_side.clone() + &format!("  {} ", LAST),
					padding,
				);
			}
			AT::NumberLiteral => {
				res += &add_header(&format!("Integer: {}\n", node.span.to_string()));
			}
			AT::FunctionType { name, .. } => {
				let s = "Function type\n".to_string();
				res += &add_header(&s);
				let s = format!("{} Name\n", LAST);
				res += &add_child(&s);
				res += &self.print_ast_helper(
					*name,
					new_side.clone() + &format!("  {} ", LAST),
					padding,
				);
				// let s = format!("Constant parameters\n");
				// res += &add(&s);
				// res += &self.print_ast_helper(c, *constant_params, side.clone() + "| ", padding);
				// let s = format!("Runtime parameters\n");
				// res += &add(&s);
				// res += &self.print_ast_helper(c, *params, side.clone() + "| ", padding);
			}
			AT::Return(id) => {
				res += &add_header("Return\n");
				res +=
					&self.print_ast_helper(*id, new_side.clone() + &format!("{} ", LAST), padding)
			}
			AT::Add { left, right } => {
				res += &add_header("Add\n");
				res += &self.print_ast_helper(
					*left,
					new_side.clone() + &format!("{} ", FORK),
					padding,
				);
				res += &self.print_ast_helper(
					*right,
					new_side.clone() + &format!("{} ", LAST),
					padding,
				);
			}
			AT::FunctionCall { function, .. } => {
				let s = "Function call\n".to_string();
				res += &add_header(&s);
				let s = format!("{} Function\n", LAST);
				res += &add_child(&s);
				res += &self.print_ast_helper(
					*function,
					new_side.clone() + &format!("  {} ", LAST),
					padding,
				);
				// let s = format!("Arguments\n");
				// res += &add(&s);
				// res += &self.print_ast_helper(c, *params, side.clone() + "| ", padding);
			}
			AT::FunctionParams() => {}
			AT::_FunctionParam() => {}
		}
		res
	}

	pub fn print_ast(&self) {
		let padding = self.ast_nodes().len().to_string().len(); // $ and :
		print!(
			"{}",
			self.print_ast_helper(
				AstId::from(self.ast_nodes().len() - 1),
				"".to_owned(),
				padding
			)
		);
	}

	pub fn enumerate_ir(&self) {
		let padding = self.ir().len().to_string().len();
		let mut level = 0usize;
		let mut enders = vec![];

		for (idx, ir) in self.ir().iter().enumerate() {
			let indent = level * 2;
			println!("%{: <padding$}: {}{}", idx, " ".repeat(indent), ir,);
			if let Ir::Block(b) = ir {
				enders.push(*b.last().unwrap_or(&0u32.into()));
				level += 1;
			}

			if enders.last().is_some_and(|e| *e <= idx) {
				enders.pop();
				level -= 1;
			}
		}
	}

	pub fn print_ir(&self) {
		let padding = self.ir().len().to_string().len();
		Self::print_ir_helper(&self.ir(), 0usize.into(), padding, 0)
	}

	fn print_ir_helper(irs: &IndexedVec<Ir>, idx: IrId, padding: usize, indent: usize) {
		let ir = &irs[idx];
		println!("%{: <padding$}: {}{}", idx, " ".repeat(indent), ir,);
		match ir {
			Ir::Block(v) => {
				for &e in v {
					Self::print_ir_helper(irs, e, padding, indent + 2);
				}
			}
			Ir::Value { value, r#type } => {
				Self::print_ir_helper(irs, *value, padding, indent + 2);
				Self::print_ir_helper(irs, *r#type, padding, indent + 2);
			}
			Ir::Add { lhs, rhs } => {
				Self::print_ir_helper(irs, *lhs, padding, indent + 2);
				Self::print_ir_helper(irs, *rhs, padding, indent + 2);
			}
			Ir::Return { value, block: _ } => {
				Self::print_ir_helper(irs, *value, padding, indent + 2);
			}
			Ir::Call { fun, args: _ } => {
				Self::print_ir_helper(irs, *fun, padding, indent + 2);
			}
			Ir::Deffun {
				iden,
				params,
				returns,
				body,
			} => {
				Self::print_ir_helper(irs, *iden, padding, indent + 2);
				Self::print_ir_helper(irs, *params, padding, indent + 2);
				Self::print_ir_helper(irs, *returns, padding, indent + 2);
				Self::print_ir_helper(irs, *body, padding, indent + 2);
			}
			_ => (),
		}
	}
}

impl Drop for Module {
	fn drop(&mut self) {
		// Undo lifetime hacks from earlier
		unsafe {
			let _ = Box::from_raw(self.source_ptr);
		}
	}
}
