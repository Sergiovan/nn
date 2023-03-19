use nn_compiler::compile;

use std::path::Path;

fn main() {
    compile(&Path::new("examples/sixty_nine.nn"));
}