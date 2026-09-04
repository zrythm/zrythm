// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

#include "utils/exceptions.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

TEST (ZrythmExceptionTest, StoresMessageFromEachConstructor)
{
  EXPECT_STREQ (ZrythmException ("char * message").what (), "char * message");
  EXPECT_STREQ (
    ZrythmException (std::string ("std::string message")).what (),
    "std::string message");
  EXPECT_STREQ (
    ZrythmException (std::string_view{ "std::string_view message" }).what (),
    "std::string_view message");
  EXPECT_STREQ (
    ZrythmException (QString::fromUtf8 ("QString message")).what (),
    "QString message");
}

TEST (ZrythmExceptionTest, WhatStringReturnsUtf8String)
{
  const ZrythmException e ("utf-8 message");
  EXPECT_EQ (
    e.what_string (), Utf8String::from_utf8_encoded_string ("utf-8 message"));
}

TEST (ZrythmExceptionTest, CaughtAsStdException)
{
  bool caught_as_std_exception = false;
  bool caught_as_runtime_error = false;
  try
    {
      throw ZrythmException ("generic catchability");
    }
  catch (const std::runtime_error &)
    {
      caught_as_runtime_error = true;
      caught_as_std_exception = true;
    }
  catch (const std::exception &)
    {
      caught_as_std_exception = true;
    }
  EXPECT_TRUE (caught_as_std_exception);
  EXPECT_TRUE (caught_as_runtime_error);
}

TEST (ZrythmExceptionTest, HandlerOrderingPrefersDerivedType)
{
  bool caught_as_zrythm_exception = false;
  try
    {
      throw ZrythmException ("derived handler must win");
    }
  catch (const ZrythmException &)
    {
      caught_as_zrythm_exception = true;
    }
  catch (const std::exception &)
    {
      caught_as_zrythm_exception = false;
    }
  EXPECT_TRUE (caught_as_zrythm_exception);
}

TEST (ZrythmExceptionTest, FormatExceptionIncludesNestedCauses)
{
  try
    {
      try
        {
          throw std::runtime_error ("disk I/O failed");
        }
      catch (const std::exception &)
        {
          std::throw_with_nested (ZrythmException ("Failed to save project"));
        }
    }
  catch (const std::exception &outer)
    {
      const auto formatted = format_exception (outer);
      EXPECT_NE (formatted.find ("Failed to save project"), std::string::npos);
      EXPECT_NE (formatted.find ("disk I/O failed"), std::string::npos);
      // the outer message comes first
      EXPECT_LT (
        formatted.find ("Failed to save project"),
        formatted.find ("disk I/O failed"));
    }
}

TEST (ZrythmExceptionTest, FormatExceptionWithoutNesting)
{
  const ZrythmException e ("plain message");
  EXPECT_EQ (format_exception (e), "plain message");
}

TEST (FormatExceptionTest, FormatExceptionOfNonZrythmNestedException)
{
  try
    {
      try
        {
          throw std::invalid_argument ("bad argument");
        }
      catch (...)
        {
          std::throw_with_nested (std::runtime_error ("wrapping context"));
        }
    }
  catch (const std::exception &outer)
    {
      const auto formatted = format_exception (outer);
      EXPECT_NE (formatted.find ("wrapping context"), std::string::npos);
      EXPECT_NE (formatted.find ("bad argument"), std::string::npos);
    }
}

} // namespace zrythm::utils
