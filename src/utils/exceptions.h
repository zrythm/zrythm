// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Exception handling utilities.
 */

#ifndef __UTILS_EXCEPTIONS_H__
#define __UTILS_EXCEPTIONS_H__

#include <exception>
#include <sstream>
#include <string>

#include <QString>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Base class for exceptions in Zrythm.
 */
class ZrythmException : public std::nested_exception
{
public:
  explicit ZrythmException (const char * message);
  explicit ZrythmException (const std::string &message);
  explicit ZrythmException (const QString &message);

  const char *      what () const noexcept;
  utils::Utf8String what_string () const
  {
    return utils::Utf8String::from_utf8_encoded_string (what ());
  }

  template <typename... Args>
  void handle (const std::string &format, Args &&... args) const;

  void handle (const char * str) const;
  void handle (const std::string &str) const;
  void handle (const QString &str) const;

private:
  std::string         message_;
  mutable std::string full_message_;
};

/**
 * @}
 */

#endif // __UTILS_EXCEPTIONS_H__
