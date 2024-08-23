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

#include "libadwaita_wrapper.h"

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
  explicit ZrythmException (const std::string &message);

  const char * what () const noexcept;

  template <typename... Args>
  AdwDialog * handle (const std::string &format, Args &&... args) const;

  AdwDialog * handle (const char * str) const;
  AdwDialog * handle (const std::string &str) const;

private:
  std::string         message_;
  mutable std::string full_message_;
};

/**
 * @}
 */

#endif // __UTILS_EXCEPTIONS_H__
