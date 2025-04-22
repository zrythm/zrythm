// SPDX-FileCopyrightText: © 2019, 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_ALGORITHMS_H__
#define __UTILS_ALGORITHMS_H__

#include <span>

#include "utils/types.h"

namespace zrythm::utils::algorithms
{

/**
 * Binary search with the option to find the closest member in a sorted array.
 *
 * @param key The value to search for
 * @param elements Span of sorted elements to search in
 * @param return_prev True to return previous closest element, false for next
 * @param include_equal Include equal elements (if exact match found, return it)
 * @return Optional reference to the found element
 */
template <std::totally_ordered T>
std::optional<std::reference_wrapper<const T>>
binary_search_nearby (
  const T           &key,
  std::span<const T> elements,
  bool               return_prev = false,
  bool               include_equal = true)
{
  // Handle empty array case
  if (elements.empty ())
    return std::nullopt;

  // Initialize binary search bounds
  size_t first = 0;
  size_t last = elements.size () - 1;

  while (first <= last)
    {
      // Calculate middle point safely to avoid overflow
      size_t   middle = first + (last - first) / 2;
      const T &pivot = elements[middle];

      // Handle exact match case
      if (pivot == key)
        {
          if (include_equal)
            return std::make_optional (std::ref (pivot));
          // Return previous/next element if exact matches not wanted
          if (return_prev)
            return middle > 0
                     ? std::make_optional (std::ref (elements[middle - 1]))
                     : std::nullopt;
          return middle < elements.size () - 1
                   ? std::make_optional (std::ref (elements[middle + 1]))
                   : std::nullopt;
        }

      // Handle case where pivot is less than search key
      if (pivot < key)
        {
          // Check if we're at the end
          if (middle == elements.size () - 1)
            return return_prev ? std::make_optional (std::ref (pivot)) : std::nullopt;
          // Found value between pivot and next element
          if (elements[middle + 1] > key)
            return return_prev
                     ? std::make_optional (std::ref (pivot))
                     : std::make_optional (std::ref (elements[middle + 1]));
          first = middle + 1;
        }
      else
        {
          // Handle case where pivot is greater than search key
          if (middle == 0)
            return return_prev
                     ? std::nullopt
                     : std::make_optional (std::ref (pivot));
          last = middle - 1;
        }
    }

  // Handle edge case when search terminates between elements
  return return_prev && last < elements.size ()
           ? std::make_optional (std::ref (elements[last]))
           : std::nullopt;
}

}; // namespace zrythm::utils

#endif
