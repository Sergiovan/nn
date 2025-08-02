#pragma once

#include <ranges>
#include <vector>

#include "assert.hpp"

template <typename T>
struct SelfReferentialContainer;

template <typename T>
struct SRContainerIndex {
  std::size_t idx;

  T& from(SelfReferentialContainer<T>& container) const;
  const T& from(const SelfReferentialContainer<T>& container) const;
};

template <typename T>
struct std::formatter<SRContainerIndex<T>> : std::formatter<std::size_t> {
  auto format(const SRContainerIndex<T>& idx, std::format_context& ctx) const {
    std::format_to(ctx.out(), "$");
    return std::formatter<std::size_t>::format(idx.idx, ctx);
  }
};

template <typename T>
struct SelfReferentialContainer {
  using Index = SRContainerIndex<T>;

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

  std::size_t size() const {
    return contents.size();
  }

  template <std::ranges::view R>
    requires std::same_as<std::ranges::range_value_t<R>, Index>
  auto iterate_content(R&& r) {
    return r | std::views::transform([this](const Index& idx) {
             return at(idx);
           });
  }

  template <std::ranges::view R>
    requires std::same_as<std::ranges::range_value_t<R>, Index>
  auto iterate_content(R&& r) const {
    return r | std::views::transform([this](const Index& idx) {
             return at(idx);
           });
  }

  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Index>
  auto iterate_content(R&& r) {
    return iterate_content(std::span{std::forward<R>(r)});
  }

  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Index>
  auto iterate_content(R&& r) const {
    return iterate_content(std::span{std::forward<R>(r)});
  }

  auto begin() {
    return contents.begin();
  }

  auto end() {
    return contents.end();
  }

  auto begin() const {
    return contents.cbegin();
  }

  auto end() const {
    return contents.cend();
  }

  auto cbegin() const {
    return contents.cbegin();
  }

  auto cend() const {
    return contents.cend();
  }

  std::vector<T> contents{};
};

template <typename T>
T& SRContainerIndex<T>::from(SelfReferentialContainer<T>& container) const {
  return container[*this];
}

template <typename T>
const T&
SRContainerIndex<T>::from(const SelfReferentialContainer<T>& container) const {
  return container[*this];
}

static_assert(std::ranges::range<const SelfReferentialContainer<int>>);
static_assert(std::ranges::viewable_range<SelfReferentialContainer<int>>);