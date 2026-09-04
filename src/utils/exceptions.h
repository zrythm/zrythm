// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

#include "utils/utf8_string.h"

#include <QString>

/**
 * @brief Exception handling utilities.
 */
namespace zrythm::utils
{

/**
 * @brief Base class for exceptions in Zrythm.
 *
 * Derived from std::runtime_error, so handlers can catch this specific type
 * or handle it generically through std::exception/std::runtime_error.
 */
class ZrythmException : public std::runtime_error
{
public:
  explicit ZrythmException (const char * message);
  explicit ZrythmException (const std::string &message);
  ZrythmException (std::string_view message);
  explicit ZrythmException (const QString &message);

  /**
   * @brief Returns the message as a UTF-8 string.
   */
  utils::Utf8String what_string () const
  {
    return utils::Utf8String::from_utf8_encoded_string (what ());
  }
};

/**
 * @brief Formats an exception into a single string.
 *
 * If the exception was thrown with std::throw_with_nested, each wrapped
 * exception's message appears on its own indented line, outermost first.
 *
 * @param exception The exception to format.
 * @return The formatted message chain.
 */
[[nodiscard]] std::string
format_exception (const std::exception &exception);

/**
 * @brief Logs an exception (and any nested exceptions) as a warning.
 *
 * @param exception The exception to log.
 * @param context Optional message describing the operation that failed,
   logged as a prefix before the exception chain.
 */
void
log_exception (const std::exception &exception, const std::string &context = {});

} // namespace zrythm::utils
