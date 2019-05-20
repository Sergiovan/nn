# nn
## Introduction
Interpreter/Compiler for a madeup language.

Check .spec for a raw overview of how the language looks like.

Check other for syntax highlighting for N++ and VS Code.

Check examples for .nn and .nna examples.

Check ROADMAP.md to see what's coming.

## Progress
* Spec: Ongoing.
  * Syntax: Done for v0.1.
  * Semantics (Casting, Name resolution, Operator precedence): Done for v0.1.
  * Parser (Mangling): Done for v0.1.
  * ASM: Who knows.
  * Data structure (Objects in memory, Program memory): Ongoing. 
  * Execution (Allocator, Function calls): Ongoing.
  * Other (TODOs, pitfalls, previous attempts): Ongoing forever.
* Lexer: Almost done.
* Parser: Mostly done.
* AST Optimizer: Stub.
* IR Generator: Needs work, basics done.
* IR Optimizer: Stub.
* IR-to-ASM compiler: Not started.
* Assembler: Needs alignment, otherwise mostly done.
* VM: Needs testing.