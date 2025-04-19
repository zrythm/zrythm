// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <iterator>
#include <type_traits>
#include <vector>

namespace zrythm::utils::collections
{

// Find the nth next occurrence matching predicate in [current+1, sentinel)
template <std::forward_iterator Iter, typename Pred>
  requires std::invocable<Pred, std::iter_reference_t<Iter>>
Iter
find_next_occurrence (Iter current, Iter sentinel, Pred pred, int n = 1)
{
  if (n < 1 || current == sentinel)
    return sentinel; // Early exit for invalid params

  for (Iter it = std::next (current); it != sentinel; ++it)
    {
      if (pred (*it) && --n == 0)
        {
          return it;
        }
    }
  return sentinel;
}

// Find the nth previous occurrence matching predicate in [begin, current)
template <std::bidirectional_iterator Iter, typename Pred>
  requires std::invocable<Pred, std::iter_reference_t<Iter>>
Iter
find_prev_occurrence (Iter current, Iter begin, Pred pred, int n = 1)
{
  if (n < 1 || current == begin)
    return begin; // Early exit for invalid params

  Iter it = current;
  do
    {
      --it;
      if (pred (*it) && --n == 0)
        {
          return it;
        }
    }
  while (it != begin);

  return begin;
}

}; // namespace zrythm::utils::collections
