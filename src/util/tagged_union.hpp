#pragma once

#include "util/assert.hpp"
#include "util/enum.hpp"

#include <utility>

namespace tagged_union_impl {

template <typename T, typename Tag>
concept has_tag = constrained_enum<Tag> && requires {
  { T::union_tag() } -> std::same_as<Tag>;
};

template <typename T, typename... Ts>
concept is_one_of = (std::same_as<T, Ts> || ...);

template <typename T, typename... Ts>
consteval std::size_t biggest_sizeof() {
  if constexpr (sizeof...(Ts) == 0) {
    return sizeof(T);
  } else {
    constexpr std::size_t biggest_t = biggest_sizeof<Ts...>();
    return (sizeof(T) > biggest_t ? sizeof(T) : biggest_t);
  }
}

template <constrained_enum Tag, Tag tag>
consteval void not_present() {}

template <constrained_enum Tag, Tag tag, has_tag<Tag> T, has_tag<Tag>... Ts>
  requires(T::union_tag() != tag)
consteval void not_present() {
  not_present<Tag, tag, Ts...>();
}

template <constrained_enum Tag, Tag tag, has_tag<Tag> T, has_tag<Tag>... Ts>
  requires(T::union_tag() == tag)
consteval void not_present() = delete;

template <constrained_enum Tag>
consteval bool all_unique() {
  return true;
}

template <constrained_enum Tag, has_tag<Tag> T, has_tag<Tag>... Ts>
consteval bool all_unique() {
  not_present<Tag, T::union_tag(), Ts...>();
  return all_unique<Tag, Ts...>();
}

template <typename TT>
struct TypeHolder {
  using T = TT;
};

template <constrained_enum Tag, Tag tag>
consteval auto type_from_tag() {
  static_assert(false);
  return TypeHolder<void>{};
}

template <constrained_enum Tag, Tag tag, has_tag<Tag> T, has_tag<Tag>... Ts>
  requires(tag == T::union_tag())
consteval auto type_from_tag() {
  return TypeHolder<T>{};
}

template <constrained_enum Tag, Tag tag, has_tag<Tag> T, has_tag<Tag>... Ts>
consteval auto type_from_tag() {
  return type_from_tag<Tag, tag, Ts...>();
}

template <constrained_enum Tag, has_tag<Tag>... Ts>
  requires(all_unique<Tag, Ts...>())
class TaggedUnion {
  struct Placeholder {
    virtual ~Placeholder() {};

    virtual void clone(void* loc) const = 0;
    virtual void move(void* loc) = 0;
  };

  template <typename T>
  struct Impl : Placeholder {
    Impl(const T& t) : t{t} {}
    Impl(T&& t) : t{std::forward<T>(t)} {}

    virtual void clone(void* loc) const {
      new (loc) Impl{t};
    }

    virtual void move(void* loc) {
      new (loc) Impl{std::move(t)};
    };

    T t;
  };

  struct Empty : Placeholder {
    virtual void clone(void* loc) const {
      new (loc) Empty;
    }

    virtual void move(void* loc) {
      new (loc) Empty;
    }
  };

  Tag tag{Tag::LAST};
  alignas(Impl<Ts>...)
      std::array<unsigned char, biggest_sizeof<Empty, Impl<Ts>...>()> mem;

public:
  template <typename T>
  static consteval bool contains_type() {
    return is_one_of<T, Ts...>;
  }

  TaggedUnion() : tag{Tag::LAST} {
    install_empty();
  }

  template <has_tag<Tag> T>
    requires is_one_of<T, Ts...>
  TaggedUnion(const T& value) {
    install(value);
  }

  ~TaggedUnion() {
    if (tag != Tag::LAST) {
      evict();
    }
  }

  TaggedUnion(const TaggedUnion& o) : TaggedUnion{} {
    if (o.tag != Tag::LAST) {
      // Destroy the Empty
      get_placeholder()->~Placeholder();
      o.get_placeholder()->clone(get_placeholder());
      tag = o.tag;
    }
  }

  TaggedUnion& operator=(const TaggedUnion& o) {
    if (this != &o) {
      evict();
      o.get_placeholder()->clone(get_placeholder());
      tag = o.tag;
    }
    return *this;
  }

  TaggedUnion(TaggedUnion&& o) : TaggedUnion{} {
    if (o.tag != Tag::LAST) {
      o.get_placeholder()->move(get_placeholder());
      tag = o.tag;
      o.evict();
      o.install_empty();
    }
  }

  TaggedUnion& operator=(TaggedUnion&& o) {
    if (this != &o) {
      o.get_placeholder()->move(get_placeholder());
      tag = o.tag;
      if (o.tag != Tag::LAST) {
        o.evict();
        o.install_empty();
      }
    }
    return *this;
  }

  template <has_tag<Tag> T>
    requires is_one_of<T, Ts...>
  T& get() {
    nn_assert(tag == T::union_tag());
    return static_cast<Impl<T>*>(get_placeholder())->t;
  }

  template <Tag tag>
  auto& get() {
    nn_assert(tag == TaggedUnion::tag);
    return get<typename decltype(type_from_tag<Tag, tag, Ts...>())::T>();
  }

  template <has_tag<Tag> T>
    requires is_one_of<T, Ts...>
  const T& get() const {
    nn_assert(tag == T::union_tag());
    return static_cast<const Impl<T>*>(get_placeholder())->t;
  }

  template <Tag tag>
  const auto& get() const {
    nn_assert(tag == TaggedUnion::tag);
    return get<typename decltype(type_from_tag<Tag, tag, Ts...>())::T>();
  }

  Tag get_tag() const {
    return tag;
  }

  template <has_tag<Tag> T>
    requires is_one_of<T, Ts...>
  void emplace(const T& t) {
    if (tag != Tag::LAST) {
      evict();
    }
    install(t);
  }

  template <has_tag<Tag> T>
  bool is_a() const {
    return TaggedUnion::tag == T::union_tag();
  }

  bool is_a(Tag tag) const {
    return TaggedUnion::tag == tag;
  }

private:
  const Placeholder* get_placeholder() const {
    return std::launder(reinterpret_cast<const Placeholder*>(&mem[0]));
  }

  Placeholder* get_placeholder() {
    return std::launder(reinterpret_cast<Placeholder*>(&mem[0]));
  }

  template <has_tag<Tag> T>
    requires is_one_of<T, Ts...>
  void install(const T& t) {
    nn_assert(tag == Tag::LAST);
    new (&mem[0]) Impl<T>{t};
    tag = T::union_tag();
  }

  void install_empty() {
    nn_assert(tag == Tag::LAST);
    new (&mem[0]) Empty;
  }

  void evict() {
    nn_assert(tag != Tag::LAST);
    get_placeholder()->~Placeholder();
    tag = Tag::LAST;
  }
};

} // namespace tagged_union_impl

using tagged_union_impl::has_tag;
using tagged_union_impl::TaggedUnion;

template <auto tag>
  requires(constrained_enum<decltype(tag)>)
struct Tagged {
  constexpr static decltype(tag) union_tag() {
    return tag;
  }
};