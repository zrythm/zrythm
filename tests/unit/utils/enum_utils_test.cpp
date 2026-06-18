// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/enum_utils.h"

#include <gtest/gtest.h>

namespace zrythm::utils::enum_test_sequential
{

enum class Color : std::uint8_t
{
  Red = 0,
  Green = 1,
  Blue = 2,
};

}

DEFINE_ENUM_FORMATTER (
  zrythm::utils::enum_test_sequential::Color,
  test_color,
  QT_TR_NOOP_UTF8 ("Red"),
  QT_TR_NOOP_UTF8 ("Green"),
  QT_TR_NOOP_UTF8 ("Blue"))

namespace zrythm::utils::enum_test_gapped
{

enum class Fruit : std::uint8_t
{
  Apple = 0,
  Banana = 3,
  Cherry = 7,
};

}

DEFINE_ENUM_FORMATTER (
  zrythm::utils::enum_test_gapped::Fruit,
  test_fruit,
  QT_TR_NOOP_UTF8 ("Apple"),
  QT_TR_NOOP_UTF8 ("Banana"),
  QT_TR_NOOP_UTF8 ("Cherry"))

namespace zrythm::utils
{

using enum_test_gapped::Fruit;
using enum_test_sequential::Color;

// --- to_string ---

TEST (EnumFormatterTest, ToStringSequential)
{
  EXPECT_EQ (test_color_to_string (Color::Red).to_qstring (), "Red");
  EXPECT_EQ (test_color_to_string (Color::Green).to_qstring (), "Green");
  EXPECT_EQ (test_color_to_string (Color::Blue).to_qstring (), "Blue");
}

TEST (EnumFormatterTest, ToStringGapped)
{
  EXPECT_EQ (test_fruit_to_string (Fruit::Apple).to_qstring (), "Apple");
  EXPECT_EQ (test_fruit_to_string (Fruit::Banana).to_qstring (), "Banana");
  EXPECT_EQ (test_fruit_to_string (Fruit::Cherry).to_qstring (), "Cherry");
}

// --- fmt::format ---

TEST (EnumFormatterTest, FmtFormatSequential)
{
  EXPECT_EQ (fmt::format ("{}", Color::Red), "Red");
  EXPECT_EQ (fmt::format ("{}", Color::Green), "Green");
  EXPECT_EQ (fmt::format ("{}", Color::Blue), "Blue");
}

TEST (EnumFormatterTest, FmtFormatGapped)
{
  EXPECT_EQ (fmt::format ("{}", Fruit::Apple), "Apple");
  EXPECT_EQ (fmt::format ("{}", Fruit::Banana), "Banana");
  EXPECT_EQ (fmt::format ("{}", Fruit::Cherry), "Cherry");
}

// --- from_string ---

TEST (EnumFormatterTest, FromStringSequential)
{
  EXPECT_EQ (
    test_color_from_string (Utf8String::from_utf8_encoded_string ("Red")),
    Color::Red);
  EXPECT_EQ (
    test_color_from_string (Utf8String::from_utf8_encoded_string ("Green")),
    Color::Green);
  EXPECT_EQ (
    test_color_from_string (Utf8String::from_utf8_encoded_string ("Blue")),
    Color::Blue);
}

TEST (EnumFormatterTest, FromStringGapped)
{
  EXPECT_EQ (
    test_fruit_from_string (Utf8String::from_utf8_encoded_string ("Apple")),
    Fruit::Apple);
  EXPECT_EQ (
    test_fruit_from_string (Utf8String::from_utf8_encoded_string ("Banana")),
    Fruit::Banana);
  EXPECT_EQ (
    test_fruit_from_string (Utf8String::from_utf8_encoded_string ("Cherry")),
    Fruit::Cherry);
}

TEST (EnumFormatterTest, FromStringThrowsOnInvalid)
{
  EXPECT_THROW (
    test_color_from_string (Utf8String::from_utf8_encoded_string ("Yellow")),
    std::runtime_error);
}

// --- count ---

TEST (EnumFormatterTest, Count)
{
  EXPECT_EQ (test_color_count (), 3u);
  EXPECT_EQ (test_fruit_count (), 3u);
}

// --- to_qml_list ---

TEST (EnumFormatterTest, ToQmlListSequential)
{
  const auto list = test_color_to_qml_list ();
  ASSERT_EQ (list.size (), 3);

  EXPECT_EQ (list[0].toMap ()["display"].toString (), "Red");
  EXPECT_EQ (list[0].toMap ()["name"].toString (), "Red");
  EXPECT_EQ (list[0].toMap ()["value"].toInt (), 0);

  EXPECT_EQ (list[1].toMap ()["display"].toString (), "Green");
  EXPECT_EQ (list[1].toMap ()["name"].toString (), "Green");
  EXPECT_EQ (list[1].toMap ()["value"].toInt (), 1);

  EXPECT_EQ (list[2].toMap ()["display"].toString (), "Blue");
  EXPECT_EQ (list[2].toMap ()["name"].toString (), "Blue");
  EXPECT_EQ (list[2].toMap ()["value"].toInt (), 2);
}

TEST (EnumFormatterTest, ToQmlListGapped)
{
  const auto list = test_fruit_to_qml_list ();
  ASSERT_EQ (list.size (), 3);

  EXPECT_EQ (list[0].toMap ()["display"].toString (), "Apple");
  EXPECT_EQ (list[0].toMap ()["value"].toInt (), 0);

  EXPECT_EQ (list[1].toMap ()["display"].toString (), "Banana");
  EXPECT_EQ (list[1].toMap ()["value"].toInt (), 3);

  EXPECT_EQ (list[2].toMap ()["display"].toString (), "Cherry");
  EXPECT_EQ (list[2].toMap ()["value"].toInt (), 7);
}

// --- round-trip ---

TEST (EnumFormatterTest, RoundTripSequential)
{
  for (size_t i = 0; i < test_color_count (); ++i)
    {
      const auto val = magic_enum::enum_value<Color> (i);
      const auto str = test_color_to_string (val);
      const auto recovered = test_color_from_string (str);
      EXPECT_EQ (recovered, val);
    }
}

TEST (EnumFormatterTest, RoundTripGapped)
{
  for (size_t i = 0; i < test_fruit_count (); ++i)
    {
      const auto val = magic_enum::enum_value<Fruit> (i);
      const auto str = test_fruit_to_string (val);
      const auto recovered = test_fruit_from_string (str);
      EXPECT_EQ (recovered, val);
    }
}

} // namespace zrythm::utils
