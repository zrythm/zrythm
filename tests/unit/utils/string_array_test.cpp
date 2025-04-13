// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/string_array.h"

TEST (StringArrayTest, Construction)
{
  // Test initializer list construction
  StringArray arr1 = { "one", "two", "three" };
  EXPECT_EQ (arr1.size (), 3);
  EXPECT_EQ (std::string (arr1[0]), "one");

  // Test QStringList construction
  QStringList qlist;
  qlist << QString::fromUtf8 ("four") << QString::fromUtf8 ("five");
  StringArray arr2 (qlist);
  EXPECT_EQ (arr2.size (), 2);
}

TEST (StringArrayTest, Modification)
{
  StringArray arr;
  arr.add ("first");
  arr.insert (0, "zero");
  EXPECT_EQ (arr.size (), 2);
  EXPECT_EQ (std::string (arr[0]), "zero");

  arr.set (1, "modified");
  EXPECT_EQ (std::string (arr[1]), "modified");

  arr.remove (0);
  EXPECT_EQ (arr.size (), 1);
}

TEST (StringArrayTest, DuplicateHandling)
{
  StringArray arr = { "one", "two", "one", "three", "two" };
  arr.removeDuplicates ();
  EXPECT_EQ (arr.size (), 3);
}

TEST (StringArrayTest, Conversion)
{
  StringArray arr = { "test1", "test2" };

  auto vec = arr.toStdStringVector ();
  EXPECT_EQ (vec.size (), 2);
  EXPECT_EQ (vec[0], "test1");

  auto qlist = arr.toQStringList ();
  EXPECT_EQ (qlist.size (), 2);
  EXPECT_EQ (qlist[0], QString::fromUtf8 ("test1"));
}

TEST (StringArrayTest, Iteration)
{
  StringArray arr = { "one", "two", "three" };
  int         count = 0;
  for (const auto &str : arr)
    {
      EXPECT_FALSE (str.isEmpty ());
      count++;
    }
  EXPECT_EQ (count, 3);
}
