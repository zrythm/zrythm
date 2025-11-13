// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <list>
#include <string>
#include <vector>

#include "utils/ranges.h"

#include <gtest/gtest.h>

namespace zrythm::ranges
{

TEST (RangesTest, AllEqualEmptyRange)
{
  std::vector<int> empty_vec;
  EXPECT_TRUE (all_equal (empty_vec));
}

TEST (RangesTest, AllEqualSingleElement)
{
  std::vector<int> single_vec = { 42 };
  EXPECT_TRUE (all_equal (single_vec));
}

TEST (RangesTest, AllEqualAllSameValues)
{
  std::vector<int> same_vec = { 5, 5, 5, 5, 5 };
  EXPECT_TRUE (all_equal (same_vec));
}

TEST (RangesTest, AllEqualDifferentValues)
{
  std::vector<int> diff_vec = { 1, 2, 3, 4, 5 };
  EXPECT_FALSE (all_equal (diff_vec));
}

TEST (RangesTest, AllEqualSomeDifferentValues)
{
  std::vector<int> mixed_vec = { 7, 7, 8, 7, 7 };
  EXPECT_FALSE (all_equal (mixed_vec));
}

TEST (RangesTest, AllEqualWithProjection)
{
  struct TestStruct
  {
    int  value;
    char name;

    auto operator<=> (const TestStruct &other) const = default;
  };

  std::vector<TestStruct> vec = {
    { 10, 'a' },
    { 10, 'b' },
    { 10, 'c' },
    { 10, 'd' }
  };

  // Test with projection to 'value' member
  EXPECT_TRUE (all_equal (vec, &TestStruct::value));

  // Test with projection to 'name' member (should be false)
  EXPECT_FALSE (all_equal (vec, &TestStruct::name));
}

TEST (RangesTest, AllEqualWithLambdaProjection)
{
  std::vector<std::string> strings = { "hello", "world", "test", "foo" };

  // Project to string length - all have different lengths
  EXPECT_FALSE (all_equal (strings, [] (const std::string &s) {
    return s.length ();
  }));

  // Project to first character - all different
  EXPECT_FALSE (all_equal (strings, [] (const std::string &s) {
    return s[0];
  }));

  // Test with same length strings
  std::vector<std::string> same_length = { "cat", "dog", "pig", "cow" };
  EXPECT_TRUE (all_equal (same_length, [] (const std::string &s) {
    return s.length ();
  }));
}

TEST (RangesTest, AllEqualArray)
{
  std::array<int, 5> arr = { 3, 3, 3, 3, 3 };
  EXPECT_TRUE (all_equal (arr));

  std::array<int, 5> arr_diff = { 1, 2, 3, 4, 5 };
  EXPECT_FALSE (all_equal (arr_diff));
}

TEST (RangesTest, AllEqualList)
{
  std::list<double> lst = { 3.14, 3.14, 3.14 };
  EXPECT_TRUE (all_equal (lst));

  std::list<double> lst_diff = { 1.0, 2.0, 3.0 };
  EXPECT_FALSE (all_equal (lst_diff));
}

TEST (RangesTest, AllEqualConstRange)
{
  const std::vector<int> const_vec = { 7, 7, 7 };
  EXPECT_TRUE (all_equal (const_vec));
}

TEST (RangesTest, AllEqualMoveSemantics)
{
  std::vector<int> vec = { 1, 1, 1 };
  EXPECT_TRUE (all_equal (std::move (vec)));
}

TEST (RangesTest, AllEqualWithIdentityProjection)
{
  std::vector<int> vec = { 42, 42, 42 };
  EXPECT_TRUE (all_equal (vec, std::identity{}));
}

TEST (RangesTest, AllEqualComplexStructures)
{
  struct Person
  {
    std::string name;
    int         age;

    bool operator== (const Person &other) const = default;
  };

  std::vector<Person> people = {
    { "Alice",   25 },
    { "Bob",     25 },
    { "Charlie", 25 },
    { "Diana",   25 }
  };

  // All have same age
  EXPECT_TRUE (all_equal (people, &Person::age));

  // Different names
  EXPECT_FALSE (all_equal (people, &Person::name));
}

TEST (RangesTest, AllEqualFloatingPoint)
{
  std::vector<float> floats = { 1.0f, 1.0f, 1.0f, 1.0f };
  EXPECT_TRUE (all_equal (floats));

  std::vector<float> floats_diff = { 1.0f, 1.000001f, 1.0f };
  EXPECT_FALSE (all_equal (floats_diff));
}

TEST (RangesTest, AllEqualBooleanValues)
{
  std::vector<bool> trues = { true, true, true, true };
  EXPECT_TRUE (all_equal (trues));

  std::vector<bool> falses = { false, false, false };
  EXPECT_TRUE (all_equal (falses));

  std::vector<bool> mixed = { true, false, true };
  EXPECT_FALSE (all_equal (mixed));
}

TEST (RangesTest, AllEqualPointers)
{
  int                a = 5, b = 5, c = 5;
  std::vector<int *> ptrs_same = { &a, &a, &a };
  EXPECT_TRUE (all_equal (ptrs_same));

  std::vector<int *> ptrs_diff = { &a, &b, &c };
  EXPECT_FALSE (all_equal (ptrs_diff));
}

TEST (RangesTest, AllEqualWithCustomComparator)
{
  std::vector<int> vec = { 1, 2, 3, 4, 5 };

  // Project to even/odd status - all should be different
  EXPECT_FALSE (all_equal (vec, [] (int x) { return x % 2; }));

  // Test with all even numbers
  std::vector<int> even_vec = { 2, 4, 6, 8, 10 };
  EXPECT_TRUE (all_equal (even_vec, [] (int x) { return x % 2; }));
}
}
