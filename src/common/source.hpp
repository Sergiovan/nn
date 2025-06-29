#pragma once

#include <memory>
#include <string>
#include <vector>

#include "util/types.hpp"

namespace source {

/** Represents one source file */
class Source {
private:
  using citerator = decltype(std::declval<std::string>().cbegin());

public:
  /** Iterator for source sub-locations (e.g. lines) */
  struct LocationIterator {
    citerator begin() const;
    citerator end() const;

    citerator _begin;
    citerator _end;
  };

  /** Constructs a source from the filename and its text */
  Source(const std::string& filename, const std::string& original);

  /** Gets the filename for this source */
  std::string_view get_filename() const;

  /** Gets the text of this source file as a string_view */
  std::string_view get() const;

  /* Iterator functions for the text of this source file */
  citerator begin() const;
  citerator end() const;

  /** Gets an iterator for the characters of the given line */
  LocationIterator get_line_iterator(u64 line);
  /** Gets the text of the given line as a string */
  std::string get_line(u64 line);

  /** Gets an iterator for `length` characters of the source starting at `pos` */
  /** Gets a substring from the source text, starting at `pos` with length `length` */
  LocationIterator get_location_iterator(u64 pos, u32 length);
  std::string get_location(u64 pos, u32 length);

private:
  /** Finds all line starts and caches them */
  void prepare_lines();

  /** File name for this source file */
  const std::string filename;
  /** Content for this source file */
  const std::string original;
  /** Source positions where a line begins */
  std::vector<u64> line_starts{};
};

/** Represents a location in some source file */
struct SourceLocation {
  /** Absolute position of the start of this location within the source file */
  u64 pos{};
  /** Line number of the start of this location within the source file */
  u32 line{};
  /** Column number of the start this location within the line of the source file */
  u32 column{};
  /** Length in characters of this source location */
  u32 length{};

  /** The source file this location belongs to */
  std::weak_ptr<Source> source{};

  std::shared_ptr<Source> get_source() const;
  /** Returns the text of this source location */
  std::string get() const;
  /** Returns the full text of the line this source location starts in */
  std::string get_full_line() const;
  /** Returns the location information as a string */
  std::string get_file_location() const;
};

/** Combines two locations that belong to the same source */
SourceLocation operator+(const SourceLocation& lhs, const SourceLocation& rhs);

const SourceLocation nullloc{{}, {}, {}, {}, {}};

} // namespace source