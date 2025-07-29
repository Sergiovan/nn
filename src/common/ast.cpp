#include "ast.hpp"
#include "common/source.hpp"

using namespace ast;

using source::SourceLocation;
using token::Token;

Ast::Ast() : data{AstNone{}} {}

const char* Ast::get_name() const {
  switch (data.get_tag()) {
    using enum Tag;
  case NONE:
    return "AstNone";
  case INTEGER:
    return "AstInteger";
  case IDENTIFIER:
    return "AstIdentifier";
  case RETURN:
    return "AstReturn";
  case PRE_OP:
    return "AstPreOp";
  case LIST:
    return "AstList";
  case FUNCTION:
    return "AstFunction";
  case LAST:
    return "AstInvalid";
  }
  unreachable;
}

std::optional<Token> Ast::main_token(const AstContainer& container) const {
  switch (data.get_tag()) {
    using enum Tag;
  case NONE:
    return std::nullopt;
  case INTEGER:
    return data.get<INTEGER>().t;
  case IDENTIFIER:
    return data.get<IDENTIFIER>().t;
  case RETURN:
    return data.get<RETURN>().t;
  case PRE_OP:
    return data.get<PRE_OP>().t;
  case LIST:
    return std::nullopt;
  case FUNCTION: {
    auto& fn = data.get<FUNCTION>();
    return fn.name.from(container).main_token(container).value_or(fn.t);
  }
  case LAST:
    return std::nullopt;
  }
  unreachable;
}

SourceLocation Ast::source_location(const AstContainer& container) const {
  switch (data.get_tag()) {
    using enum Tag;
  case NONE:
    return source::nullloc;
  case INTEGER:
    return data.get<INTEGER>().t.loc;
  case IDENTIFIER:
    return data.get<IDENTIFIER>().t.loc;
  case RETURN: {
    auto& ret = data.get<RETURN>();
    return ret.t.loc + ret.child.from(container).source_location(container);
  }
  case PRE_OP: {
    auto& pre_op = data.get<PRE_OP>();
    return pre_op.t.loc +
           pre_op.child.from(container).source_location(container);
  }
  case LIST: {
    auto& list = data.get<LIST>();
    if (list.asts.empty()) {
      return source::nullloc;
    } else if (list.asts.size() == 1) {
      return list.asts.front().from(container).source_location(container);
    } else {
      return list.asts.front().from(container).source_location(container) +
             list.asts.back().from(container).source_location(container);
    }
  }
  case FUNCTION: {
    auto& fn = data.get<FUNCTION>();
    return fn.t.loc + fn.body.from(container).source_location(container);
  }
  case LAST:
    return source::nullloc;
  }
  unreachable;
}

Tag Ast::get_tag() const {
  return data.get_tag();
}

bool Ast::is_a(Tag tag) const {
  return data.get_tag() == tag;
}

void Ast::require(Tag tag) const {
  nn_assert(is_a(tag));
}
