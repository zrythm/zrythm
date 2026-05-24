// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <ranges>
#include <utility>

#include <QObject>

namespace zrythm::utils::views
{

namespace detail
{

/** Enumerates a range with index pairs (index, element). */
struct EnumerateAdaptor : std::ranges::range_adaptor_closure<EnumerateAdaptor>
{
  template <std::ranges::viewable_range Range> auto operator() (Range &&r) const
  {
    return std::views::zip (
      std::views::iota (std::size_t{ 0 }), std::forward<Range> (r));
  }
};

/** Filters out null pointers from a range of pointers. */
struct FilterNullAdaptor : std::ranges::range_adaptor_closure<FilterNullAdaptor>
{
  template <std::ranges::viewable_range Range> auto operator() (Range &&r) const
  {
    return std::forward<Range> (r) | std::views::filter ([] (const auto &ptr) {
             return ptr != nullptr;
           });
  }
};

/**
 * @brief Transforms a range of QObject pointers via qobject_cast.
 *
 * Null results are NOT filtered — use qobject_cast_and_filter for that.
 */
template <typename Derived>
struct QObjectCastToAdaptor
    : std::ranges::range_adaptor_closure<QObjectCastToAdaptor<Derived>>
{
  template <std::ranges::viewable_range Range> auto operator() (Range &&r) const
  {
    return std::forward<Range> (r)
           | std::views::transform ([] (auto * ptr) -> Derived * {
               return qobject_cast<Derived *> (ptr);
             });
  }
};

/**
 * @brief qobject_cast + filter_null combined: returns only non-null cast
 * results.
 */
template <typename Derived>
struct QObjectCastAndFilterAdaptor
    : std::ranges::range_adaptor_closure<QObjectCastAndFilterAdaptor<Derived>>
{
  template <std::ranges::viewable_range Range> auto operator() (Range &&r) const
  {
    return std::forward<Range> (r) | detail::QObjectCastToAdaptor<Derived>{}
           | detail::FilterNullAdaptor{};
  }
};

} // namespace detail

inline constexpr detail::EnumerateAdaptor enumerate{};

inline constexpr detail::FilterNullAdaptor filter_null{};

template <typename Derived>
inline constexpr detail::QObjectCastToAdaptor<Derived> qobject_cast_to{};

template <typename Derived>
inline constexpr detail::QObjectCastAndFilterAdaptor<Derived>
  qobject_cast_and_filter{};

} // namespace zrythm::utils::views
