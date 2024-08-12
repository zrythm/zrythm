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
  explicit ZrythmException (const std::string &message) : message_ (message) { }

  const char * what () const noexcept
  {
    try
      {
        std::rethrow_exception (std::current_exception ());
      }
    catch (const std::exception &e)
      {
        std::ostringstream oss;
        oss << "Exception Stack:\n";
        oss << "- " << message_ << "\n";
        std::exception_ptr nested = std::nested_exception::nested_ptr ();
        while (nested)
          {
            try
              {
                std::rethrow_exception (nested);
              }
            catch (const std::exception &nested_e)
              {
                oss << "- " << nested_e.what () << "\n";
                nested = std::current_exception ();
              }
          }
        full_message_ = oss.str ();
        return full_message_.c_str ();
      }
  }

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
