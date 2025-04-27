// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <iterator>
#include <utility>

namespace zrythm::utils::views
{

template <typename Iterator> class EnumerateIterator
{
public:
  using iterator_category =
    typename std::iterator_traits<Iterator>::iterator_category;
  using value_type =
    std::pair<std::size_t, typename std::iterator_traits<Iterator>::value_type>;
  using difference_type =
    typename std::iterator_traits<Iterator>::difference_type;
  using pointer =
    std::pair<std::size_t, typename std::iterator_traits<Iterator>::pointer>;
  using reference =
    std::pair<std::size_t, typename std::iterator_traits<Iterator>::reference>;

  using ValueType =
    std::pair<std::size_t, decltype (*std::declval<Iterator> ())>;

  EnumerateIterator (Iterator it, std::size_t index = 0)
      : index_ (index), it_ (it)
  {
  }

  ValueType operator* () const { return { index_, *it_ }; }

  EnumerateIterator &operator++ ()
  {
    ++index_;
    ++it_;
    return *this;
  }
  EnumerateIterator operator++ (int)
  {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator== (const EnumerateIterator &other) const
  {
    return it_ == other.it_;
  }
  bool operator!= (const EnumerateIterator &other) const
  {
    return !(*this == other);
  }

  bool operator== (const Iterator &other) const { return it_ == other; }

private:
  std::size_t index_;
  Iterator    it_;
};

template <typename Range>
class EnumerateView : public std::ranges::view_interface<EnumerateView<Range>>
{
  Range range_;

public:
  using iterator = EnumerateIterator<std::ranges::iterator_t<Range>>;
  using sentinel = EnumerateIterator<std::ranges::sentinel_t<Range>>;

  explicit EnumerateView (Range &&r) : range_ (std::forward<Range> (r)) { }

  auto begin ()
  {
    using std::begin;
    return EnumerateIterator (begin (range_), 0);
  }

  auto end ()
  {
    using std::end;
    return EnumerateIterator (end (range_));
  }
};

// Helper function to create the view
template <std::ranges::viewable_range Range>
auto
enumerate (Range &&r)
{
  return EnumerateView<Range> (std::forward<Range> (r));
}

} // namespace zrythm::utils::views