// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/units.h"

#include <fmt/format.h>

namespace zrythm::dsp
{

/**
 * @brief Absolute positions on the project's musical timeline.
 *
 * Convert from ContentTick ONLY via ContentTimeWarp::contentToTimeline().
 *
 * au deliberately does not support "kind" types (same dimension, same
 * magnitude, different logical domain).  This wrapper provides that safety:
 * BasicTimelineTick and BasicContentTick are unrelated types with no
 * implicit conversion between them, so mixing them is a compile error.
 *
 * Cross-precision conversion (int→double within the same domain) delegates
 * entirely to au: au allows implicit int→double (lossless).  Narrowing
 * (double→int) is not provided — use `.asDouble()` if you need the raw
 * value.
 *
 * @tparam Q The underlying au tick quantity type (e.g. @ref
 * units::precise_tick_t for double precision, @ref units::tick_t for integer
 * precision).
 */
template <typename Q> struct BasicTimelineTick
{
  Q value_{};

  // --- Default special members ---
  constexpr BasicTimelineTick () = default;
  constexpr BasicTimelineTick (const BasicTimelineTick &) = default;
  constexpr BasicTimelineTick (BasicTimelineTick &&) noexcept = default;
  constexpr BasicTimelineTick &operator= (const BasicTimelineTick &) = default;
  constexpr BasicTimelineTick &
  operator= (BasicTimelineTick &&) noexcept = default;

  /// Construct from any au quantity convertible to Q.
  template <typename Q2>
    requires std::convertible_to<Q2, Q>
  constexpr explicit BasicTimelineTick (Q2 q) : value_ (std::move (q))
  {
  }

  // --- Cross-precision constructor (delegates to au) ---

  /// Implicit widening (e.g. int→double) — au allows it.
  template <typename Q2>
    requires (!std::same_as<Q, Q2>) && std::convertible_to<const Q2 &, Q>
  constexpr BasicTimelineTick (const BasicTimelineTick<Q2> &other)
      : value_ (other.value_)
  {
  }

  // --- Comparisons (same type only — prevents cross-domain mixing) ---
  bool operator== (const BasicTimelineTick &) const = default;
  auto operator<=> (const BasicTimelineTick &) const = default;

  // --- Arithmetic (same type only) ---
  BasicTimelineTick operator+ (const BasicTimelineTick &o) const
  {
    return BasicTimelineTick{ value_ + o.value_ };
  }
  BasicTimelineTick operator- (const BasicTimelineTick &o) const
  {
    return BasicTimelineTick{ value_ - o.value_ };
  }
  BasicTimelineTick &operator+= (const BasicTimelineTick &o)
  {
    value_ += o.value_;
    return *this;
  }
  BasicTimelineTick &operator-= (const BasicTimelineTick &o)
  {
    value_ -= o.value_;
    return *this;
  }
  BasicTimelineTick operator- () const { return BasicTimelineTick{ -value_ }; }

  // --- Accessors ---
  Q      asQuantity () const { return value_; }
  double asDouble () const { return value_.in (units::ticks); }
};

/// Double-precision timeline ticks (the default for most code).
using TimelineTick = BasicTimelineTick<units::precise_tick_t>;

/// Integer-precision timeline ticks (for musical positions that are always
/// whole numbers).
using TimelineTickI = BasicTimelineTick<units::tick_t>;

/**
 * @brief Positions relative to a clip's data origin (0 = clip start).
 *
 * Convert to TimelineTick ONLY via ContentTimeWarp::contentToTimeline().
 *
 * @tparam Q The underlying au tick quantity type.
 */
template <typename Q> struct BasicContentTick
{
  Q value_{};

  // --- Default special members ---
  constexpr BasicContentTick () = default;
  constexpr BasicContentTick (const BasicContentTick &) = default;
  constexpr BasicContentTick (BasicContentTick &&) noexcept = default;
  constexpr BasicContentTick &operator= (const BasicContentTick &) = default;
  constexpr BasicContentTick &
  operator= (BasicContentTick &&) noexcept = default;

  /// Construct from any au quantity convertible to Q.
  template <typename Q2>
    requires std::convertible_to<Q2, Q>
  constexpr explicit BasicContentTick (Q2 q) : value_ (std::move (q))
  {
  }

  // --- Cross-precision constructor (delegates to au) ---

  /// Implicit widening (e.g. int→double) — au allows it.
  template <typename Q2>
    requires (!std::same_as<Q, Q2>) && std::convertible_to<const Q2 &, Q>
  constexpr BasicContentTick (const BasicContentTick<Q2> &other)
      : value_ (other.value_)
  {
  }

  // --- Comparisons (same type only — prevents cross-domain mixing) ---
  bool operator== (const BasicContentTick &) const = default;
  auto operator<=> (const BasicContentTick &) const = default;

  // --- Arithmetic (same type only) ---
  BasicContentTick operator+ (const BasicContentTick &o) const
  {
    return BasicContentTick{ value_ + o.value_ };
  }
  BasicContentTick operator- (const BasicContentTick &o) const
  {
    return BasicContentTick{ value_ - o.value_ };
  }
  BasicContentTick &operator+= (const BasicContentTick &o)
  {
    value_ += o.value_;
    return *this;
  }
  BasicContentTick &operator-= (const BasicContentTick &o)
  {
    value_ -= o.value_;
    return *this;
  }
  BasicContentTick operator- () const { return BasicContentTick{ -value_ }; }

  // --- Accessors ---
  Q      asQuantity () const { return value_; }
  double asDouble () const { return value_.in (units::ticks); }
};

/// Double-precision content ticks (the default).
using ContentTick = BasicContentTick<units::precise_tick_t>;

/// Integer-precision content ticks.
using ContentTickI = BasicContentTick<units::tick_t>;

// --- Concept and helpers ---

namespace detail
{
template <typename> struct is_basic_timeline_tick : std::false_type
{
};
template <typename Q>
struct is_basic_timeline_tick<BasicTimelineTick<Q>> : std::true_type
{
};
template <typename> struct is_basic_content_tick : std::false_type
{
};
template <typename Q>
struct is_basic_content_tick<BasicContentTick<Q>> : std::true_type
{
};
}

template <typename T>
concept TickType =
  detail::is_basic_timeline_tick<std::remove_cvref_t<T>>::value
  || detail::is_basic_content_tick<std::remove_cvref_t<T>>::value;

template <TickType T>
T
abs (T t)
{
  return t < T{} ? -t : t;
}

} // namespace zrythm::dsp

// Formatters — write value directly to output buffer, then append domain label.
// Inherits parse() from fmt::formatter<double> so format specs like {:>10.2f}
// apply to the numeric part.
template <typename U>
struct fmt::formatter<::zrythm::dsp::BasicTimelineTick<U>>
    : fmt::formatter<double>
{
  template <typename FormatContext>
  constexpr auto
  format (const ::zrythm::dsp::BasicTimelineTick<U> &t, FormatContext &ctx) const
  {
    auto out = fmt::formatter<double>::format (t.asDouble (), ctx);
    return fmt::format_to (out, " timeline ticks");
  }
};

template <typename U>
struct fmt::formatter<::zrythm::dsp::BasicContentTick<U>> : fmt::formatter<double>
{
  template <typename FormatContext>
  constexpr auto
  format (const ::zrythm::dsp::BasicContentTick<U> &t, FormatContext &ctx) const
  {
    auto out = fmt::formatter<double>::format (t.asDouble (), ctx);
    return fmt::format_to (out, " content ticks");
  }
};

static_assert (fmt::formattable<::zrythm::dsp::TimelineTick>);
static_assert (fmt::formattable<::zrythm::dsp::ContentTick>);
