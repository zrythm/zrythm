// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace zrythm::utils
{

/**
 * @brief Move-only callable wrapper that always stores the callable inline
 * (never allocates).
 *
 * Similar to `std::function`, except:
 * - the callable is stored in an internal buffer of @p inline_capacity
 *   bytes, so construction never allocates on the heap;
 * - it is move-only, and stored callables are only required to be nothrow
 *   move-constructible;
 * - callables that do not fit in the buffer, are over-aligned, or can throw
 *   on move are rejected at compile time (`std::is_constructible_v` reflects
 *   whether a callable type can be stored).
 *
 * Intended for passing callbacks across threads through lock-free queues,
 * where `std::function` is unusable because it allocates when the callable
 * exceeds its small-object buffer.
 *
 * Like `std::function`, invoking the stored callable through a const
 * reference is allowed and invokes it as a non-const lvalue (mutable
 * lambdas work).
 *
 * @tparam inline_capacity Size of the inline storage in bytes.
 */
template <typename Signature, std::size_t inline_capacity = 48>
class InplaceFunction;

template <typename R, typename... Args, std::size_t inline_capacity>
class InplaceFunction<R (Args...), inline_capacity>
{
public:
  /// Creates an empty (non-callable) instance.
  InplaceFunction () noexcept = default;
  InplaceFunction (std::nullptr_t) noexcept : InplaceFunction () { }

  /**
   * @brief Stores @p func in the internal buffer.
   *
   * Participates in overload resolution only if the decayed type of @p func
   * is invocable with this signature, fits in the internal buffer, is not
   * over-aligned, is nothrow-constructible from @p func (its copy
   * constructor when @p func is an lvalue, otherwise its move
   * constructor), and is nothrow move-constructible (for later moves
   * between instances).
   */
  template <typename F>
    requires (
      !std::is_same_v<std::decay_t<F>, InplaceFunction>
      && std::is_invocable_r_v<R, std::decay_t<F> &, Args...>
      && std::is_nothrow_constructible_v<std::decay_t<F>, F &&>
      && std::is_nothrow_move_constructible_v<std::decay_t<F>>
      && sizeof (std::decay_t<F>) <= inline_capacity
      && alignof (std::decay_t<F>) <= alignof (std::max_align_t))
  InplaceFunction (F &&func) noexcept
  {
    using Func = std::decay_t<F>;
    std::construct_at (
      reinterpret_cast<Func *> (storage_.data ()), std::forward<F> (func));
    invoke_ = [] (void * storage, Args... args) -> R {
      return std::invoke (
        *std::launder (reinterpret_cast<Func *> (storage)),
        std::forward<Args> (args)...);
    };
    destroy_ = [] (void * storage) noexcept {
      std::destroy_at (std::launder (reinterpret_cast<Func *> (storage)));
    };
    move_ = [] (void * dst, void * src) noexcept {
      std::construct_at (
        reinterpret_cast<Func *> (dst),
        std::move (*std::launder (reinterpret_cast<Func *> (src))));
    };
  }

  InplaceFunction (const InplaceFunction &) = delete;
  InplaceFunction &operator= (const InplaceFunction &) = delete;

  InplaceFunction (InplaceFunction &&other) noexcept { move_from (other); }

  InplaceFunction &operator= (InplaceFunction &&other) noexcept
  {
    if (this != &other)
      {
        reset ();
        move_from (other);
      }
    return *this;
  }

  InplaceFunction &operator= (std::nullptr_t) noexcept
  {
    reset ();
    return *this;
  }

  ~InplaceFunction () { reset (); }

  /**
   * @brief Invokes the stored callable.
   *
   * @pre The instance is not empty.
   */
  R operator() (Args... args) const
  {
    assert (invoke_ != nullptr);
    return invoke_ (storage_.data (), std::forward<Args> (args)...);
  }

  /// Whether a callable is stored.
  explicit operator bool () const noexcept { return invoke_ != nullptr; }

  /// Destroys the stored callable, making the instance empty.
  void reset () noexcept
  {
    if (destroy_ != nullptr)
      {
        destroy_ (storage_.data ());
      }
    invoke_ = nullptr;
    destroy_ = nullptr;
    move_ = nullptr;
  }

private:
  void move_from (InplaceFunction &other) noexcept
  {
    if (other.move_ == nullptr)
      return;
    other.move_ (storage_.data (), other.storage_.data ());
    invoke_ = other.invoke_;
    destroy_ = other.destroy_;
    move_ = other.move_;
    other.reset ();
  }

  using InvokeFn = R (*) (void *, Args...);
  using DestroyFn = void (*) (void *) noexcept;
  using MoveFn = void (*) (void *, void *) noexcept;

  InvokeFn  invoke_ = nullptr;
  DestroyFn destroy_ = nullptr;
  MoveFn    move_ = nullptr;

  alignas (
    std::max_align_t) mutable std::array<std::byte, inline_capacity> storage_{};
};

} // namespace zrythm::utils
