#pragma once

#include <vector>

#include "assert.hpp"

template <typename T>
struct SRContainerIndex {
  std::size_t idx;
};

template <typename T, typename IndexGuard = T>
struct SelfReferentialContainer {
  using Index = SRContainerIndex<IndexGuard>;

  T& at(Index idx) {
    nn_assert(contents.size() > idx.idx);
    return contents[idx.idx];
  }

  T& operator[](Index idx) {
    return at(idx);
  }

  const T& at(Index idx) const {
    nn_assert(contents.size() > idx.idx);
    return contents[idx.idx];
  }

  const T& operator[](Index idx) const {
    return at(idx);
  }

  Index push_back(const T& elem) {
    contents.push_back(elem);
    return Index{contents.size() - 1};
  }

  template <typename... Args>
  Index emplace_back(Args&&... args) {
    contents.emplace_back(std::forward<Args>(args)...);
  }

  std::vector<T> contents{};
};