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
  case LIST:
    return std::nullopt;
  case FUNCTION: {
    auto& fn = data.get<FUNCTION>();
    return container[fn.name].main_token(container).value_or(fn.t);
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
    return ret.t.loc + container[ret.child].source_location(container);
  }
  case LIST: {
    auto& list = data.get<LIST>();
    if (list.asts.empty()) {
      return source::nullloc;
    } else if (list.asts.size() == 1) {
      return container[list.asts.front()].source_location(container);
    } else {
      return container[list.asts.front()].source_location(container) +
             container[list.asts.back()].source_location(container);
    }
  }
  case FUNCTION: {
    auto& fn = data.get<FUNCTION>();
    return fn.t.loc + container[fn.body].source_location(container);
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
