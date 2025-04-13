// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/algorithms.h"
#include "utils/gtest_wrapper.h"

TEST (AlgorithmsTest, NearbyBinarySearch)
{
  struct IntWrapper
  {
    explicit IntWrapper (int v) : value (v) { }
    int value;

    bool operator< (const IntWrapper &other) const
    {
      return value < other.value;
    }

    auto operator<=> (const IntWrapper &other) const = default;
  };

  // Test exact matches
  {
    std::array<IntWrapper, 10> arr = {
      IntWrapper (0), IntWrapper (1), IntWrapper (2), IntWrapper (3),
      IntWrapper (4), IntWrapper (5), IntWrapper (6), IntWrapper (7),
      IntWrapper (8), IntWrapper (9)
    };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (5), std::span<const IntWrapper> (arr), true, true);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 5);
  }

  // Test previous neighbor
  {
    std::array<IntWrapper, 10> arr = {
      IntWrapper (0), IntWrapper (1), IntWrapper (2), IntWrapper (3),
      IntWrapper (4), IntWrapper (6), IntWrapper (7), IntWrapper (8),
      IntWrapper (9), IntWrapper (10)
    };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (5), std::span<const IntWrapper> (arr), true, true);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 4);
  }

  // Test next neighbor
  {
    std::array<IntWrapper, 10> arr = {
      IntWrapper (0), IntWrapper (1), IntWrapper (2), IntWrapper (3),
      IntWrapper (4), IntWrapper (6), IntWrapper (7), IntWrapper (8),
      IntWrapper (9), IntWrapper (10)
    };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (5), std::span<const IntWrapper> (arr), false, true);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 6);
  }

  // Test empty array
  {
    std::array<IntWrapper, 0> arr = {};
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (5), std::span<const IntWrapper> (arr), true, true);
    EXPECT_FALSE (ret.has_value ());
  }

  // Test single element array
  {
    std::array<IntWrapper, 1> arr = { IntWrapper (5) };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (5), std::span<const IntWrapper> (arr), true, true);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 5);
  }

  // Test edge cases - searching before first element
  {
    std::array<IntWrapper, 5> arr = {
      IntWrapper (1), IntWrapper (2), IntWrapper (3), IntWrapper (4),
      IntWrapper (5)
    };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (0), std::span<const IntWrapper> (arr), false, true);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 1);
  }

  // Test edge cases - searching after last element
  {
    std::array<IntWrapper, 5> arr = {
      IntWrapper (1), IntWrapper (2), IntWrapper (3), IntWrapper (4),
      IntWrapper (5)
    };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (6), std::span<const IntWrapper> (arr), true, true);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 5);
  }

  // Test include_equal=false
  {
    std::array<IntWrapper, 5> arr = {
      IntWrapper (1), IntWrapper (2), IntWrapper (3), IntWrapper (4),
      IntWrapper (5)
    };
    auto ret = zrythm::utils::algorithms::binary_search_nearby (
      IntWrapper (3), std::span<const IntWrapper> (arr), true, false);
    ASSERT_TRUE (ret.has_value ());
    EXPECT_EQ (ret->get ().value, 2);
  }
}
