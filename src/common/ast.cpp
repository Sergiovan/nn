#include "ast.hpp"

using namespace ast;

using source::SourceLocation;
using token::Token;

Ast::Ast() : data{AstNone{}} {}

AstBase& Ast::get() {
  return visit([](AstBase& s) -> auto& {
    return s;
  });
}

const char* Ast::get_name() {
  return visit([](AstLike auto& ast) -> const char* {
    return ast.get_name();
  });
}

std::optional<Token> Ast::main_token() {
  return visit([](AstLike auto& ast) -> std::optional<Token> {
    return ast.main_token();
  });
}

SourceLocation Ast::source_location() {
  return visit([](AstLike auto& ast) -> SourceLocation {
    auto res = ast.source_location();
    return res;
  });
}

AstPtr Ast::as_ptr() && {
  return std::make_unique<Ast>(std::move(*this));
}

std::optional<Token> AstNone::main_token() {
  return std::nullopt;
}

SourceLocation AstNone::source_location() {
  return source::nullloc;
}

AstToken::AstToken(Token t) : t{t} {}

std::optional<Token> AstToken::main_token() {
  return t;
}

SourceLocation AstToken::source_location() {
  return t.loc;
}

AstUnary::AstUnary(Token t, Ast&& other)
    : t{t}, child{std::move(other).as_ptr()} {}

std::optional<Token> AstUnary::main_token() {
  return t;
}

SourceLocation AstUnary::source_location() {
  return t.loc + child->source_location();
}

std::optional<Token> AstBinary::main_token() {
  return t;
}

SourceLocation AstBinary::source_location() {
  return lhs->source_location() + rhs->source_location();
}

AstBinary::AstBinary(Token t, Ast&& lhs, Ast&& rhs)
    : t{t}, lhs{std::move(lhs).as_ptr()}, rhs{std::move(rhs).as_ptr()} {}

AstList::AstList() : asts{} {}

AstList::AstList(std::vector<AstPtr>&& asts) : asts{std::move(asts)} {}

std::optional<Token> AstList::main_token() {
  return std::nullopt;
}

SourceLocation AstList::source_location() {
  if (asts.empty()) {
    return source::nullloc;
  } else if (asts.size() == 1) {
    return asts.front()->source_location();
  } else {
    return asts.front()->source_location() + asts.back()->source_location();
  }
}
AstFunction::AstFunction(Token t, Ast&& name, Ast&& body)
    : t{t}, name{std::move(name).as_ptr()}, body{std::move(body).as_ptr()} {}

std::optional<Token> AstFunction::main_token() {
  return name->main_token().value_or(t);
}

SourceLocation AstFunction::source_location() {
  return t.loc + body->source_location();
}