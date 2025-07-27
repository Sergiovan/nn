#pragma once

#include <memory>
#include <optional>

#include "common/asm_ast.hpp"
#include "common/ast.hpp"

namespace asm_parser {
/* Converts AST into ASM-AST */

struct AsmBlock {
  std::vector<std::unique_ptr<asm_ast::AstInstruction>>* instructions;

  void add(std::unique_ptr<asm_ast::AstInstruction> instruction);
};

class AsmParser {
public:
  AsmParser(const ast::Ast& ast_in, const ast::AstContainer& ast_container);

  const asm_ast::AstProgram& get();

private:
  /* We recurse descentively, again */
  /* parse_ functions take in an ast of the given type */
  /* gen_ functions generate an asm ast of the given type */
  asm_ast::AstProgram program(const ast::Ast& ast);
  asm_ast::AstFunctionDef parse_function_definition(const ast::Ast& ast);
  void parse_list(const ast::Ast& ast);
  void parse_return(const ast::Ast& ast);

  std::unique_ptr<asm_ast::AstInstructionMov>
  gen_mov(std::unique_ptr<asm_ast::AstOperandRegister> src,
          std::unique_ptr<asm_ast::AstOperandRegister> dst);
  std::unique_ptr<asm_ast::AstInstructionLi>
  gen_li(std::unique_ptr<asm_ast::AstOperandImmediate> src,
         std::unique_ptr<asm_ast::AstOperandRegister> dst);
  std::unique_ptr<asm_ast::AstOperandImmediate>
  gen_integer(const ast::Ast& ast);

  std::optional<AsmBlock> add_block(
      std::vector<std::unique_ptr<asm_ast::AstInstruction>>& instructions);
  std::optional<AsmBlock> add_block(std::optional<AsmBlock>& block);
  void add_to_block(std::unique_ptr<asm_ast::AstInstruction> inst);

  const ast::Ast& ast_in;
  const ast::AstContainer& ast_container;

  std::optional<asm_ast::AstProgram> head{std::nullopt};
  std::optional<AsmBlock> current_block{std::nullopt};
};
} // namespace asm_parser