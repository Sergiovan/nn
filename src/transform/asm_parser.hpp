#pragma once

#include "common/asm_ast.hpp"
#include "common/tac.hpp"

namespace asm_parser {
struct ParseResult {
  asm_ast::Program program;

  const asm_ast::InstructionContainer& instructions;
  const asm_ast::OperandContainer& operands;
};

/* Converts TAC into ASM-AST */
class AsmParser {
public:
  AsmParser(const tac::Tac& tac_in, const tac::TacContainer& tac_container);

  ParseResult parse();

private:
  asm_ast::Program program(const tac::Tac& tac);
  asm_ast::Function function(const tac::Tac& tac);

  /* Returns the stack usage in bytes */
  uint64_t fix_pseudos(asm_ast::Function& fn);
  /* Function prologue */
  void add_scaffolding(asm_ast::Function& fn, uint64_t stack_size);
  /* Makes sure operands on instructions are valid */
  void fix_operands(asm_ast::Function& fn);

  bool suitable_instruction(tac::Tag tag);
  asm_ast::InstructionIndex
  instruction(const tac::Tac& tac, std::vector<asm_ast::InstructionIndex>& loc);

  asm_ast::OperandIndex get_operand(asm_ast::InstructionIndex idx);

  bool suitable_operand(tac::Tag tag);
  asm_ast::OperandIndex operand(const tac::Tac& tac);

  /* Takes a TAC that could be instruction or operand. If operand, returns that. Otherwise
     adds the instructions and returns an operand for whatever the last instruction's
     result corresponds to */
  asm_ast::OperandIndex
  forced_operand(const tac::Tac& tac,
                 std::vector<asm_ast::InstructionIndex>& loc);

  asm_ast::InstructionIndex add_instruction(const asm_ast::Instruction& inst);
  asm_ast::OperandIndex add_operand(const asm_ast::Operand& op);

  tac::Tac tac_top;
  const tac::TacContainer& tac_container;

  asm_ast::InstructionContainer instructions;
  asm_ast::OperandContainer operands;
};
} // namespace asm_parser