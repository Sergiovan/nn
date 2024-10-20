#include "ast.hpp"

using namespace ast;

Ast::Ast() : data{AstNone{}} {}

AstBase& Ast::get() {
  return visit([](AstBase& s) -> auto& {
    return s;
  });
}

const char* Ast::get_name() {
  return visit([](AstLike auto& ast) {
    return ast.get_name();
  });
}

AstPtr Ast::as_ptr() && {
  return std::make_unique<Ast>(std::move(*this));
}

AstToken::AstToken(token::Token t) : t{t} {}

AstUnary::AstUnary(Ast&& other) : child{std::move(other).as_ptr()} {}

AstBinary::AstBinary(Ast&& lhs, Ast&& rhs)
    : lhs{std::move(lhs).as_ptr()}, rhs{std::move(rhs).as_ptr()} {}

AstList::AstList() : asts{} {}

AstList::AstList(std::vector<AstPtr>&& asts) : asts{std::move(asts)} {}

AstFunction::AstFunction(Ast&& name, Ast&& body)
    : name{std::move(name).as_ptr()}, body{std::move(body).as_ptr()} {}