// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cassert>
#include <functional>
#include <optional>

namespace zrythm::utils
{

/**
 * @brief Wrapper around std::optional<std::reference_wrapper<T>> that provides
 * a more convenient API.
 *
 * This class provides a convenient wrapper around
 * std::optional<std::reference_wrapper<T>> that allows you to easily access the
 * underlying value without having to check for the presence of a value first.
 *
 * The `operator*()` and `value()` methods return a reference to the underlying
 * value, and the `has_value()` method can be used to check if a value is
 * present.
 */
template <typename T> struct OptionalRef
{
  OptionalRef () = default;
  OptionalRef (std::nullopt_t) { }
  OptionalRef (T &ref) : ref_ (ref) { }

  std::optional<std::reference_wrapper<T>> ref_;

  /**
   * @brief Dereference the underlying value.
   *
   * @return A reference to the underlying value.
   */
  T &operator* ()
  {
    assert (has_value ());
    return ref_->get ();
  }
  const T &operator* () const
  {
    assert (has_value ());
    return ref_->get ();
  }

  const T * operator->() const
  {
    assert (has_value ());
    return std::addressof (ref_->get ());
  }
  T * operator->()
  {
    assert (has_value ());
    return std::addressof (ref_->get ());
  }

  explicit operator bool () const { return has_value (); }

  /**
   * @brief Check if a value is present.
   *
   * @return `true` if a value is present, `false` otherwise.
   */
  bool has_value () const { return ref_.has_value (); }

  /**
   * @brief Get a reference to the underlying value.
   *
   * @return A reference to the underlying value.
   */
  T &value ()
  {
    assert (has_value ());
    return ref_->get ();
  }
};
} // namespace zrythm::utils
