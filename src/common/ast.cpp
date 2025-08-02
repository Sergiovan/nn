#include "ast.hpp"
#include "common/source.hpp"

using namespace ast;

using source::SourceLocation;
using token::Token;

const char* Ast::get_name() const {
  switch (get_tag()) {
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
  switch (get_tag()) {
    using enum Tag;
  case NONE:
    return std::nullopt;
  case INTEGER:
    return get<INTEGER>().t;
  case IDENTIFIER:
    return get<IDENTIFIER>().t;
  case RETURN:
    return get<RETURN>().t;
  case PRE_OP:
    return get<PRE_OP>().t;
  case LIST:
    return std::nullopt;
  case FUNCTION: {
    auto& fn = get<FUNCTION>();
    return fn.name.from(container).main_token(container).value_or(fn.t);
  }
  case LAST:
    return std::nullopt;
  }
  unreachable;
}

SourceLocation Ast::source_location(const AstContainer& container) const {
  switch (get_tag()) {
    using enum Tag;
  case NONE:
    return source::nullloc;
  case INTEGER:
    return get<INTEGER>().t.loc;
  case IDENTIFIER:
    return get<IDENTIFIER>().t.loc;
  case RETURN: {
    auto& ret = get<RETURN>();
    return ret.t.loc + ret.child.from(container).source_location(container);
  }
  case PRE_OP: {
    auto& pre_op = get<PRE_OP>();
    return pre_op.t.loc +
           pre_op.child.from(container).source_location(container);
  }
  case LIST: {
    auto& list = get<LIST>();
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
    auto& fn = get<FUNCTION>();
    return fn.t.loc + fn.body.from(container).source_location(container);
  }
  case LAST:
    return source::nullloc;
  }
  unreachable;
}

void Ast::require(Tag tag) const {
  nn_assert(is_a(tag));
}
