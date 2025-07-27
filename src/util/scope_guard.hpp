#pragma once

#include <type_traits>

template <typename F>
  requires(std::is_invocable_r_v<void, F>)
struct ScopeGuard {
  ScopeGuard(F&& f) : f{f} {}

  ~ScopeGuard() {
    f();
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  F f;
};