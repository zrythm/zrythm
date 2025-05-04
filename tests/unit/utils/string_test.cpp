// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/string.h"

#include <QtEnvironmentVariables>

using namespace zrythm::utils;
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
  qputenv ("TEST_VAR", "test_value");
  EXPECT_EQ (expand_env_vars ("${TEST_VAR}"), "test_value");
  qunsetenv ("TEST_VAR");
}

TEST (StringTest, StringConversions)
{
  // juce::String <-> std::string
  {
    juce::String juceStr (juce::CharPointer_UTF8 ("Test 日本語"));
    std::string  stdStr = juce_string_to_std_string (juceStr);
    EXPECT_EQ (stdStr, "Test 日本語");

    juce::String convertedBack = string_view_to_juce_string (stdStr);
    EXPECT_EQ (convertedBack, juceStr);
  }

  // QString <-> std::string
  {
    QString     qStr ("Test 😊");
    std::string stdStr = qstring_to_std_string (qStr);
    EXPECT_EQ (stdStr, "Test 😊");

    QString convertedBack = std_string_to_qstring (stdStr);
    EXPECT_EQ (convertedBack, qStr);
  }

  // juce::String <-> QString
  {
    juce::String juceStr (juce::CharPointer_UTF8 ("混合文本"));
    QString      qStr = juce_string_to_qstring (juceStr);
    EXPECT_EQ (qStr, "混合文本");
  }
}

TEST (StringTest, PathConversions)
{
  // Unix-style paths
  {
    QUrl unixUrl ("file:///home/user/文件.txt");
    EXPECT_EQ (qurl_to_path_qstring (unixUrl), "/home/user/文件.txt");
  }

  // Windows-style paths
  {
    QUrl    winUrl ("file:///C:/Users/用户/文档.txt");
    QString path = qurl_to_path_qstring (winUrl);
#ifdef _WIN32
    EXPECT_EQ (path, "C:/Users/用户/文档.txt");
#else
    EXPECT_EQ (
      path, "/C:/Users/用户/文档.txt"); // QUrl normalizes to absolute path
#endif
  }

  // Network paths
  {
    QUrl netUrl ("file://server/share/测试.txt");
    EXPECT_EQ (qurl_to_path_qstring (netUrl), "//server/share/测试.txt");
  }

  // Relative paths
  {
    QUrl relUrl ("file:../relative/路径");
    EXPECT_EQ (qurl_to_path_qstring (relUrl), "../relative/路径");
  }
}

TEST (StringTest, EdgeCaseConversions)
{
  // Empty strings
  {
    EXPECT_TRUE (juce_string_to_std_string (juce::String ()).empty ());
    EXPECT_TRUE (qstring_to_std_string (QString ()).empty ());
    EXPECT_TRUE (juce_string_to_qstring (juce::String ()).isEmpty ());
  }
}