#include "tac_parser.hpp"
#include "common/tac.hpp"
#include "frontend/parser.hpp"
#include "util/assert.hpp"

using namespace tac_parser;

using tac::TacIndex;

TacParser::TacParser(parser::ParseResult ast, error::ErrorManager& errors)
    : ast_in{ast.top}, ast_container{ast.container}, errors{errors},
      container{} {}

ParseResult TacParser::parse() {
  ast_in.require<ast::AstList>();

  TacIndex out = program(ast_in);

  return {out.from(container), container};
}

TacIndex TacParser::program(const ast::Ast& ast) {
  ast.require<ast::AstList>();

  const auto& list = ast.get<ast::AstList>();

  for (const auto& fn : ast_container.iterate_content(list.asts)) {
    switch (fn.get_tag()) {
      using enum ast::Tag;
    case FUNCTION:
      return add_tac(tac::TacProgram{function_definition(fn)});
    default:
      unreachable;
    }
  }
  unreachable;
}

TacIndex TacParser::function_definition(const ast::Ast& ast) {
  ast.require<ast::AstFunction>();
  const auto& fn_ast = ast.get<ast::AstFunction>();

  // TODO Simplify?
  std::string fn_name =
      fn_ast.name.from(ast_container).main_token(ast_container)->loc.get();
  tac::TacFunction fn{fn_name};

  for (const auto& inst : ast_container.iterate_content(
           fn_ast.body.from(ast_container).get<ast::AstList>().asts)) {
    fn.instructions.push_back(instruction(inst));
  }

  return add_tac(fn);
}

TacIndex TacParser::instruction(const ast::Ast& ast) {
  switch (ast.get_tag()) {
    using enum ast::Tag;
  case RETURN: {
    const auto& ret = ast.get<RETURN>();
    return add_tac(tac::TacReturn{val(ret.child.from(ast_container))});
  } break;
  default:
    unreachable;
  }
}

TacIndex TacParser::val(const ast::Ast& ast) {
  switch (ast.get_tag()) {
    using enum ast::Tag;
  case INTEGER: {
    const auto& integer = ast.get<INTEGER>();
    int64_t integer_value = stoll(integer.t.loc.get());
    return add_tac(tac::TacConstant{integer_value});
  } break;
  case IDENTIFIER: {
    const auto& iden = ast.get<IDENTIFIER>();
    return add_tac(tac::TacIdentifier{iden.t.loc.get()});
  } break;
  case PRE_OP: {
    const auto& unary = ast.get<PRE_OP>();
    return add_tac(
        tac::TacUnary(unary.t.tt, val(unary.child.from(ast_container))));
  } break;
  default:
    unreachable;
  }
}

TacIndex TacParser::add_tac(const tac::Tac& tac) {
  return container.push_back(tac);
}