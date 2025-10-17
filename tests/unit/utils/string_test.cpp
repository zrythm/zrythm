// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/utf8_string.h"

#include <QtEnvironmentVariables>

using namespace Qt::StringLiterals;

using namespace zrythm::utils;

TEST (StringTest, EscapeHtml)
{
  EXPECT_EQ (
    Utf8String::from_utf8_encoded_string ("<test>").escape_html (),
    "&lt;test&gt;");
  EXPECT_EQ (
    Utf8String::from_utf8_encoded_string ("\"quote\"").escape_html (),
    "&quot;quote&quot;");
  EXPECT_EQ (
    Utf8String::from_utf8_encoded_string ("&ampersand").escape_html (),
    "&amp;ampersand");
}

TEST (StringTest, IsAscii)
{
  EXPECT_TRUE (Utf8String::from_utf8_encoded_string ("Hello123!").is_ascii ());
  EXPECT_FALSE (Utf8String::from_utf8_encoded_string ("Hello世界").is_ascii ());
}

TEST (StringTest, ContainsSubstring)
{
  EXPECT_TRUE (
    Utf8String::from_utf8_encoded_string ("Hello World")
      .contains_substr ({ u8"World" }));
  EXPECT_TRUE (
    Utf8String{ u8"Hello World" }.contains_substr_case_insensitive (
      { u8"WORLD" }));
  EXPECT_FALSE (
    Utf8String::from_utf8_encoded_string ("Hello World")
      .contains_substr ({ u8"Goodbye" }));
}

TEST (StringTest, CaseConversion)
{
  EXPECT_EQ (Utf8String{ u8"Hello123" }.to_upper (), "HELLO123");
  EXPECT_EQ (Utf8String{ u8"HELLO123" }.to_lower (), "hello123");
}

TEST (StringTest, GetIntAfterLastSpace)
{
  auto [num, str] = Utf8String (u8"Hello World 42").get_int_after_last_space ();
  EXPECT_EQ (num, 42);
  EXPECT_EQ (str, "Hello World");

  std::tie (num, str) =
    Utf8String (u8"No number here").get_int_after_last_space ();
  EXPECT_EQ (num, -1);
}

TEST (StringTest, RegexOperations)
{
  EXPECT_EQ (
    Utf8String{ u8"Hello, World!" }.get_regex_group (u8"(World)", 1), "World");
  EXPECT_EQ (
    Utf8String (u8"Age: 25 years")
      .get_regex_group_as_int (u8"Age: (\\d+)", 1, -1),
    25);
}

TEST (StringTest, PathManipulation)
{
  EXPECT_EQ (
    Utf8String (u8"Hello World!").convert_to_filename (), "Hello_World_");
  EXPECT_EQ (
    Utf8String (u8"test.txt").get_substr_before_suffix ({ u8".txt" }), "test");
}

TEST (StringTest, StringJoin)
{
  std::vector<Utf8String> vec = { u8"one", u8"two", u8"three" };
  EXPECT_EQ (Utf8String::join (vec, u8", "), "one, two, three");
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
  EXPECT_EQ (Utf8String (u8"${TEST_VAR}").expand_env_vars (), "test_value");
  qunsetenv ("TEST_VAR");
}

TEST (StringTest, StringConversions)
{
  // juce::String <-> std::string
  {
    juce::String juceStr (juce::CharPointer_UTF8 ("Test 日本語"));
    auto         u8str = Utf8String::from_juce_string (juceStr);
    const auto  &stdStr = u8str.str ();
    EXPECT_EQ (stdStr, "Test 日本語");

    juce::String convertedBack =
      Utf8String::from_utf8_encoded_string (stdStr).to_juce_string ();
    EXPECT_EQ (convertedBack, juceStr);
  }

  // QString <-> std::string
  // note: emojis don't work
  {
    QString    qStr (u"Test 日本語"_s);
    const auto stdStr = Utf8String::from_qstring (qStr).str ();
    EXPECT_EQ (stdStr, "Test 日本語");

    QString convertedBack =
      Utf8String::from_utf8_encoded_string (stdStr).to_qstring ();
    EXPECT_EQ (convertedBack, qStr);
  }

  // juce::String <-> QString
  {
    juce::String juceStr (juce::CharPointer_UTF8 ("混合文本"));
    QString      qStr = Utf8String::from_juce_string (juceStr).to_qstring ();
    EXPECT_EQ (qStr, u"混合文本"_s);
  }
}

TEST (StringTest, PathConversions)
{
  // Unix-style paths
  {
    QUrl unixUrl ("file:///home/user/文件.txt");
    EXPECT_EQ (Utf8String::from_qurl (unixUrl), "/home/user/文件.txt");
  }

  // Windows-style paths
  {
    QUrl       winUrl ("file:///C:/Users/用户/文档.txt");
    const auto path = Utf8String::from_qurl (winUrl);
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
    EXPECT_EQ (Utf8String::from_qurl (netUrl), "//server/share/测试.txt");
  }

  // Relative paths
  {
    QUrl relUrl ("file:../relative/路径");
    EXPECT_EQ (Utf8String::from_qurl (relUrl), "../relative/路径");
  }
}

TEST (StringTest, EdgeCaseConversions)
{
  // Empty strings
  {
    EXPECT_TRUE (Utf8String::from_juce_string (juce::String ()).empty ());
    EXPECT_TRUE (Utf8String::from_qstring (QString ()).empty ());
    EXPECT_TRUE (
      Utf8String::from_juce_string (juce::String ()).to_qstring ().isEmpty ());
  }
}
