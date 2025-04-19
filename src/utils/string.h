// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_STRING_H__
#define __UTILS_STRING_H__

#include "utils/traits.h"

#include <QString>

#include <fmt/format.h>

namespace juce
{
class String;
}

/**
 * @brief String utilities.
 */
namespace zrythm::utils::string
{

std::string
escape_html (const std::string &str);

juce::String
string_view_to_juce_string (std::string_view sv);

/**
 * Returns if the string is ASCII.
 */
bool
is_ascii (std::string_view string);

/**
 * Returns if the given string contains the given substring.
 */
bool
contains_substr (const char * str, const char * substr);

bool
contains_substr_case_insensitive (const char * str, const char * substr);

/**
 * @brief Converts only ASCII characters to uppercase.
 *
 * @param in
 * @return std::string
 */
void
to_upper_ascii (std::string &str);

/**
 * @brief Converts only ASCII characters to lowercase.
 *
 * @param in
 * @return std::string
 */
void
to_lower_ascii (std::string &str);

/**
 * Returns if the two strings are exactly equal.
 */
#define string_is_equal(str1, str2) \
  (std::string_view (str1) == std::string_view (str2))

/**
 * Returns if the two strings are equal ignoring case.
 */
bool
is_equal_ignore_case (const std::string &str1, const std::string &str2);

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI Region #1" -> "MIDI_Region_1".
 */
std::string
convert_to_filename (const std::string &str);

/**
 * Removes the suffix starting from @ref suffix
 * from @ref full_str and returns a newly allocated
 * string.
 */
std::string
get_substr_before_suffix (const std::string &str, const std::string &suffix);

/**
 * Removes everything up to and including the first match of @ref match from the
 * start of the string.
 */
std::string
remove_until_after_first_match (const std::string &str, const std::string &match);

std::string
replace (const std::string &str, const std::string &from, const std::string &to);

/**
 * Gets the string in the given regex group.
 *
 * @return The string, or an empty string if nothing found.
 */
std::string
get_regex_group (const std::string &str, const std::string &regex, int group);

/**
 * Gets the string in the given regex group as an
 * integer.
 *
 * @param def Default.
 *
 * @return The int, or default.
 */
int
get_regex_group_as_int (
  const std::string &str,
  const std::string &regex,
  int                group,
  int                def);

/**
 * Returns the integer found at the end of a string like "My String 3" -> 3,
 * or -1 if no number is found.
 *
 * See https://www.debuggex.com/cheatsheet/regex/pcre for more info.
 *
 * @return The integer found at the end of the string, or -1 if no number is
 * found, and the string without the number (including the space).
 */
std::pair<int, std::string>
get_int_after_last_space (const std::string &str);

/**
 * TODO
 * Sorts the given string array and removes
 * duplicates.
 *
 * @param str_arr A NULL-terminated array of strings.
 *
 * @return A NULL-terminated array with the string
 *   addresses of the source array.
 */
char **
array_sort_and_remove_duplicates (char ** str_arr);

/**
 * Returns a new string with only ASCII alphanumeric
 * characters and replaces the rest with underscore.
 */
std::string
symbolify (const std::string &in);

/**
 * Returns whether the string is NULL or empty.
 */
bool
is_empty (const char * str);

/**
 * Expands environment variables enclosed in ${} in the given string.
 */
std::string
expand_env_vars (const std::string &src);

#include <cstdlib>
#include <utility>

class CStringRAII
{
public:
  // Constructor that takes ownership of the given string
  CStringRAII (char * str) noexcept : str_ (str) { }

  // Destructor
  ~CStringRAII () { free (str_); }

  // Delete copy constructor and assignment operator
  CStringRAII (const CStringRAII &) = delete;
  CStringRAII &operator= (const CStringRAII &) = delete;

  // Move constructor
  CStringRAII (CStringRAII &&other) noexcept
      : str_ (std::exchange (other.str_, nullptr))
  {
  }

  // Move assignment operator
  CStringRAII &operator= (CStringRAII &&other) noexcept
  {
    if (this != &other)
      {
        free (str_);
        str_ = std::exchange (other.str_, nullptr);
      }
    return *this;
  }

  // Getter for the underlying C-string
  [[nodiscard]] const char * c_str () const noexcept { return str_; }

  bool empty () const noexcept { return !str_ || strlen (str_) == 0; }

private:
  char * str_;
};

std::string
join (const std::vector<std::string> &strings, std::string_view delimiter);

}; // namespace zrythm::utils::string

#endif
