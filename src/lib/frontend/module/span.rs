use super::ModuleId;

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