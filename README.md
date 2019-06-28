# nn
## Introduction
Interpreter/Compiler for a madeup language.

Check [src](src/) to check out the source code.

Check [.spec](.spec/) for a raw overview of how the language looks like.

Check [other](other/) for syntax highlighting for N++ and VS Code.

Check [examples](examples/) for .nn and .nna examples.

Check [ROADMAP.md](ROADMAP.md) to see what's coming.

## Progress
* Spec: Ongoing.
  * Syntax: Done for v0.1.
  * Semantics (Casting, Name resolution, Operator precedence): Done for v0.1.
  * Parser (Mangling): Done for v0.1.
  * ASM: Who knows.
  * Data structure (Objects in memory, Program memory): Ongoing. 
  * Execution (Allocator, Function calls): Ongoing.
  * Other (TODOs, pitfalls, previous attempts): Ongoing forever.
* [Lexer](src/frontend/lexer.cpp): Almost done.
* [Parser](src/frontend/parser.cpp): Mostly done.
* [AST Optimizer](src/frontend/ast_optimizer.cpp): Stub.
* [IR Generator](src/frontend/ast_to_ir.cpp): Needs work, basics done.
* [IR Optimizer](src/frontend/ast_to_ir.cpp): Stub. 
* [IR-to-nnASM compiler](src/backend/ir_compiler.cpp): Not started.
* [Assembler](src/frontend/nnasm_compiler.cpp): One more rework. Last one, I promise.
* [VM](src/vm/machine.cpp): Needs testing and retouching.
