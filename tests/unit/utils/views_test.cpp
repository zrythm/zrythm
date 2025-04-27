// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <list>
#include <vector>

#include "utils/gtest_wrapper.h"
#include "utils/views.h"

using namespace zrythm::utils::views;

TEST (ViewsTest, EnumerateVector)
{
  std::vector<int> vec = { 10, 20, 30 };
  std::size_t      expected_index = 0;

  for (const auto &[index, value] : enumerate (vec))
    {
      EXPECT_EQ (index, expected_index);
      EXPECT_EQ (value, vec[index]);
      expected_index++;
    }

  EXPECT_EQ (expected_index, vec.size ());
}

TEST (ViewsTest, EnumerateArray)
{
  std::array<std::string, 3> arr = { "one", "two", "three" };
  std::size_t                expected_index = 0;

  for (const auto &[index, value] : enumerate (arr))
    {
      EXPECT_EQ (index, expected_index);
      EXPECT_EQ (value, arr[index]);
      expected_index++;
    }

  EXPECT_EQ (expected_index, arr.size ());
}

TEST (ViewsTest, EnumerateList)
{
  std::list<char> lst = { 'a', 'b', 'c' };
  std::size_t     expected_index = 0;

  for (const auto &[index, value] : enumerate (lst))
    {
      EXPECT_EQ (index, expected_index);
      expected_index++;
    }

  EXPECT_EQ (expected_index, lst.size ());
}

TEST (ViewsTest, EnumerateEmptyContainer)
{
  std::vector<int> empty_vec;
  bool             loop_entered = false;

  for (const auto &[index, value] : enumerate (empty_vec))
    {
      (void) index;
      (void) value;
      loop_entered = true;
    }

  EXPECT_FALSE (loop_entered);
}

TEST (ViewsTest, EnumerateModification)
{
  std::vector<int> vec = { 1, 2, 3 };

  for (const auto &[index, value] : enumerate (vec))
    {
      value *= 2;
    }

  EXPECT_EQ (vec[0], 2);
  EXPECT_EQ (vec[1], 4);
  EXPECT_EQ (vec[2], 6);
}

#if 0
TEST (ViewsTest, EnumerateReverse)
{
  std::vector<int> vec = { 10, 20, 30 };
  std::size_t      expected_index = vec.size () - 1;

  for (const auto &[index, value] : enumerate (vec) | std::views::reverse)
    {
      EXPECT_EQ (index, expected_index);
      EXPECT_EQ (value, vec[index]);
      expected_index--;
    }
}

TEST (ViewsTest, EnumerateFilter)
{
  std::vector<int> vec = { 1, 2, 3, 4, 5 };
  std::size_t      count = 0;

  auto is_even = [] (const auto &pair) { return pair.second % 2 == 0; };

  for (
    const auto &[index, value] : enumerate (vec) | std::views::filter (is_even))
    {
      EXPECT_EQ (value % 2, 0);
      count++;
    }

  EXPECT_EQ (count, 2); // 2 and 4 are even
}

TEST (ViewsTest, EnumerateTransform)
{
  std::vector<int>         vec = { 1, 2, 3 };
  std::vector<std::string> expected = { "1-0", "2-1", "3-2" };

  auto transform = [] (const auto &pair) {
    return std::to_string (pair.second) + "-" + std::to_string (pair.first);
  };

  auto transformed_view = enumerate (vec) | std::views::transform (transform);

  std::vector<std::string> results;
  for (const auto &str : transformed_view)
    {
      results.push_back (str);
    }

  EXPECT_EQ (results, expected);
}

TEST (ViewsTest, EnumerateTake)
{
  std::vector<int> vec = { 1, 2, 3, 4, 5 };
  std::size_t      count = 0;

  for (const auto &[index, value] : enumerate (vec) | std::views::take (2))
    {
      EXPECT_LT (index, 2);
      count++;
    }

  EXPECT_EQ (count, 2);
}
#endif