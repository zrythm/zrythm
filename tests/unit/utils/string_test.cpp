// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/string.h"

using namespace zrythm::utils::string;

TEST (StringTest, EscapeHtml)
{
  EXPECT_EQ (escape_html ("<test>"), "&lt;test&gt;");
  EXPECT_EQ (escape_html ("\"quote\""), "&quot;quote&quot;");
  EXPECT_EQ (escape_html ("&ampersand"), "&amp;ampersand");
}

TEST (StringTest, IsAscii)
{
  EXPECT_TRUE (is_ascii ("Hello123!"));
  EXPECT_FALSE (is_ascii ("Hello世界"));
}

TEST (StringTest, ContainsSubstring)
{
  EXPECT_TRUE (contains_substr ("Hello World", "World"));
  EXPECT_TRUE (contains_substr_case_insensitive ("Hello World", "WORLD"));
  EXPECT_FALSE (contains_substr ("Hello World", "Goodbye"));
}

TEST (StringTest, CaseConversion)
{
  std::string str = "Hello123";
  to_upper_ascii (str);
  EXPECT_EQ (str, "HELLO123");

  str = "HELLO123";
  to_lower_ascii (str);
  EXPECT_EQ (str, "hello123");
}

TEST (StringTest, GetIntAfterLastSpace)
{
  auto [num, str] = get_int_after_last_space ("Hello World 42");
  EXPECT_EQ (num, 42);
  EXPECT_EQ (str, "Hello World");

  std::tie (num, str) = get_int_after_last_space ("No number here");
  EXPECT_EQ (num, -1);
}

TEST (StringTest, RegexOperations)
{
  EXPECT_EQ (get_regex_group ("Hello, World!", "(World)", 1), "World");
  EXPECT_EQ (get_regex_group_as_int ("Age: 25 years", "Age: (\\d+)", 1, -1), 25);
}

TEST (StringTest, PathManipulation)
{
  EXPECT_EQ (convert_to_filename ("Hello World!"), "Hello_World_");
  EXPECT_EQ (get_substr_before_suffix ("test.txt", ".txt"), "test");
}

TEST (StringTest, StringJoin)
{
  std::vector<std::string> vec = { "one", "two", "three" };
  EXPECT_EQ (join (vec, ", "), "one, two, three");
}

TEST (StringTest, CStringRAII)
{
  {
    CStringRAII str (strdup ("test"));
    EXPECT_STREQ (str.c_str (), "test");
    EXPECT_FALSE (str.empty ());
  }

  {
    CStringRAII empty_str (nullptr);
    EXPECT_TRUE (empty_str.empty ());
  }
}

TEST (StringTest, EnvironmentVariables)
{
  setenv ("TEST_VAR", "test_value", 1);
  EXPECT_EQ (expand_env_vars ("${TEST_VAR}"), "test_value");
  unsetenv ("TEST_VAR");
}
