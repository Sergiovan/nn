use super::ModuleId;

#[derive(Copy, Clone, Debug)]
pub struct Span {
	pub text: &'static str,
	pub module_idx: ModuleId,
	pub start: u32,
	pub line: u32,
	pub col: u32
}

impl ToString for Span {
	fn to_string(&self) -> String {
		self.text.to_owned()
	}
}