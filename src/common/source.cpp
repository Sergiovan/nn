#include "source.hpp"

#include "util/assert.hpp"

using namespace source;

Source::Source(const std::string& filename, const std::string& original)
    : filename{filename}, original{original} {}

std::string_view Source::get_filename() const {
  return filename;
}

std::string_view Source::get() const {
  return original;
}

Source::citerator Source::LocationIterator::begin() const {
  return _begin;
}

Source::citerator Source::LocationIterator::end() const {
  return _end;
}

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

std::shared_ptr<Source> SourceLocation::get_source() const {
  return source.lock();
}

std::string SourceLocation::get() const {
  auto shared = source.lock();

  if (shared) {
    return shared->get_location(pos, length);
  } else {
    return {};
  }
}

std::string SourceLocation::get_full_line() const {
  auto shared = source.lock();

  if (shared) {
    return shared->get_line(line);
  } else {
    return {};
  }
}

std::string SourceLocation::get_file_location() const {
  auto shared = source.lock();

  if (shared) {
    return std::format("{} [line: {}, col: {}]", shared->get_filename(),
                       line + 1, column + 1);
  } else {
    return std::format("???????.?? [line: {}, col: {}]", line + 1, column + 1);
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