use super::ModuleId;

#[derive(Copy, Clone, Debug)]
pub struct Span {
	pub text: &'static str,
	pub _module_idx: ModuleId,
	pub start: u32,
	pub line: u32,
	pub col: u32,
}

impl std::fmt::Display for Span {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		self.text.fmt(f)
	}
}
