#include "source.hpp"

#include "util/assert.hpp"

using namespace source;

std::string_view Source::get() const {
  return original;
}

Source::citerator Source::LocationIterator::begin() const {
  return _begin;
}

Source::citerator Source::LocationIterator::end() const {
  return _end;
}

Source::Source(const std::string& original) : original{original} {}

Source::citerator Source::begin() const {
  return original.cbegin();
}

Source::citerator Source::end() const {
  return original.cend();
}

Source::LocationIterator Source::get_line_iterator(u64 line) {
  if (line_starts.empty()) {
    prepare_lines();
  }

  nn_assert(line + 1 < line_starts.size());

  return {begin() + line_starts[line], begin() + line_starts[line + 1] - 1};
}

std::string Source::get_line(u64 line) {
  auto it = get_line_iterator(line);
  return {it.begin(), it.end()};
}

Source::LocationIterator Source::get_location_iterator(u64 pos, u32 length) {
  nn_assert(pos < original.length());
  nn_assert(pos + length <= original.length());

  return {begin() + pos, begin() + pos + length};
}

std::string Source::get_location(u64 pos, u32 length) {
  auto it = get_location_iterator(pos, length);
  return {it.begin(), it.end()};
}

void Source::prepare_lines() {
  line_starts.push_back(0);

  const Source& self = *this;

  u64 index{0};
  for (char c : self) {
    if (c == '\n') { // The only newline character >:(
      line_starts.push_back(index + 1);
    }

    index++;
  }

  line_starts.push_back(index + 1);
}

std::string SourceLocation::get() const {
  auto shared = source.lock();

  if (shared) {
    return shared->get_location(pos, length);
  } else {
    return {};
  }
}

SourceLocation source::operator+(const SourceLocation& lhs,
                                 const SourceLocation& rhs) {
  auto lhs_source = lhs.source.lock();
  auto rhs_source = rhs.source.lock();

  nn_assert(lhs_source == rhs_source);

  if (lhs_source) {
    if (lhs.pos < rhs.pos) {
      return {lhs.pos, lhs.line, lhs.column,
              (u32)((rhs.pos + rhs.length) - lhs.pos), lhs.source};
    } else {
      return {rhs.pos, rhs.line, rhs.column,
              (u32)((lhs.pos + lhs.length) - rhs.pos), rhs.source};
    }
  } else {
    return nullloc;
  }
}