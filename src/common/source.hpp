#pragma once

#include <memory>
#include <string>
#include <vector>

#include "util/types.hpp"

namespace source {

class Source {
private:
  using citerator = decltype(std::declval<std::string>().cbegin());

public:
  struct LocationIterator {
    citerator begin() const;
    citerator end() const;

    citerator _begin;
    citerator _end;
  };

  Source(const std::string& original);

  std::string_view get() const;

  citerator begin() const;
  citerator end() const;

  LocationIterator get_line_iterator(u64 line);
  std::string get_line(u64 line);

  LocationIterator get_location_iterator(u64 pos, u32 length);
  std::string get_location(u64 pos, u32 length);

private:
  void prepare_lines();

  const std::string original;
  std::vector<u64> line_starts{};
};

struct SourceLocation {
  u64 pos{};
  u32 line{};
  u32 column{};
  u32 length{};

  std::weak_ptr<Source> source{};

  std::string get() const;
};

SourceLocation operator+(const SourceLocation& lhs, const SourceLocation& rhs);

const SourceLocation nullloc{{}, {}, {}, {}, {}};

} // namespace source