#pragma once

#include "common/ast.hpp"
#include "common/error.hpp"
#include "common/tac.hpp"
#include "frontend/parser.hpp"

namespace tac_parser {

struct ParseResult {
  tac::Tac top;
  const tac::TacContainer& container;
};

class TacParser {
public:
  TacParser(parser::ParseResult ast, error::ErrorManager& errors);

  ParseResult parse();

private:
  tac::TacIndex program(const ast::Ast& ast);
  tac::TacIndex function_definition(const ast::Ast& ast);
  tac::TacIndex instruction(const ast::Ast& ast);
  tac::TacIndex val(const ast::Ast& ast);
  // unary_operator is just a symbol

  tac::TacIndex add_tac(const tac::Tac& tac);

  const ast::Ast ast_in;
  const ast::AstContainer& ast_container;

  error::ErrorManager& errors;
  tac::TacContainer container;
};

} // namespace tac_parser