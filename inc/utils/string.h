// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 * Copyright (C) 2012 Intel Corporation
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Rodrigo Moya <rodrigo@ximian.com>
 *          Tristan Van Berkom <tristanvb@openismus.com>
 *
 * ---
 */

/**
 * @file
 *
 * String utilities.
 */

#ifndef __UTILS_STRING_H__
#define __UTILS_STRING_H__

#include <format>

#include "ext/juce/juce.h"
#include <fmt/format.h>
#include <glibmm.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * String array that auto-converts given char pointers to UTF8 (so JUCE doesn't
 * complain.
 */
class StringArray
{
public:
  StringArray () {};
  StringArray (const std::initializer_list<const char *> &_strings)
  {
    arr_ = juce::StringArray (_strings);
  }
  StringArray (const std::initializer_list<std::string> &_strings)
  {
    arr_ = juce::StringArray (_strings);
  }
  StringArray (const char * const * strs);

  /**
   * @brief Returns the strings in a newly-allocated NULL-terminated array.
   *
   * To be used for C APIs.
   */
  char ** getNullTerminated () const;

  void insert (int index, const char * s)
  {
    arr_.insert (index, juce::CharPointer_UTF8 (s));
  };
  void   add (const std::string &s) { arr_.add (s); }
  size_t size () const { return arr_.size (); }
  void   add (const char * s) { arr_.add (juce::CharPointer_UTF8 (s)); };
  void   set (int index, const char * s)
  {
    arr_.set (index, juce::CharPointer_UTF8 (s));
  };
  char *       getCStr (int index);
  juce::String getReference (int index) const
  {
    return arr_.getReference (index);
  }
  const char * operator[] (size_t i)
  {
    return arr_.getReference (i).toRawUTF8 ();
  };
  void removeString (const char * s)
  {
    arr_.removeString (juce::CharPointer_UTF8 (s));
  };

  void removeDuplicates (bool ignore_case = false)
  {
    arr_.removeDuplicates (ignore_case);
  }

  void addArray (const StringArray &other) { arr_.addArray (other.arr_); };

  void remove (int index) { arr_.remove (index); };

  bool isEmpty () const { return arr_.isEmpty (); };
  bool contains (const juce::StringRef &s, bool ignore_case = false) const
  {
    return arr_.contains (s, ignore_case);
  }

  std::vector<std::string> toStdStringVector () const
  {
    std::vector<std::string> ret;
    for (auto &s : arr_)
      {
        ret.push_back (s.toStdString ());
      }
    return ret;
  }

  void print (std::string title) const;

  // Iterator support
  auto begin () { return arr_.begin (); }
  auto end () { return arr_.end (); }
  auto begin () const { return arr_.begin (); }
  auto end () const { return arr_.end (); }

  // Range-based for loop support
  auto &operator* () { return *this; }
  auto &operator++ () { return *this; }
  bool  operator!= (const StringArray &) const { return false; }

private:
  juce::StringArray arr_;
};

Glib::ustring
string_view_to_ustring (std::string_view sv);

juce::String
string_view_to_juce_string (std::string_view sv);

/**
 * Returns if the string is ASCII.
 */
bool
string_is_ascii (std::string_view string);

/**
 * Returns the matched string if the string array contains the given substring.
 */
char *
string_array_contains_substr (char ** str_array, int num_str, const char * substr);

/**
 * Returns if the given string contains the given substring.
 */
bool
string_contains_substr (const char * str, const char * substr);

bool
string_contains_substr_case_insensitive (const char * str, const char * substr);

/**
 * Converts the given string to uppercase in @ref out.
 *
 * Assumes @ref out is already allocated to as many chars as @ref in.
 */
void
string_to_upper (const char * in, char * out);

/**
 * Converts the given string to lowercase in @ref out.
 *
 * Assumes @ref out is already allocated to as many chars as @ref in.
 */
void
string_to_lower (const char * in, char * out);

/**
 * Returns if the two strings are exactly equal.
 */
#define string_is_equal(str1, str2) (!g_strcmp0 (str1, str2))

/**
 * Returns if the two strings are equal ignoring case.
 */
bool
string_is_equal_ignore_case (const char * str1, const char * str2);

bool
string_is_equal_ignore_case (const std::string &str1, const std::string &str2);

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI Region #1" -> "MIDI_Region_1".
 */
NONNULL char *
string_convert_to_filename (const char * str);

/**
 * Removes the suffix starting from @ref suffix
 * from @ref full_str and returns a newly allocated
 * string.
 */
MALLOC
NONNULL char *
string_get_substr_before_suffix (const char * str, const char * suffix);

/**
 * Removes everything up to and including the first
 * match of @ref match from the start of the string
 * and returns a newly allocated string.
 */
char *
string_remove_until_after_first_match (const char * str, const char * match);

/**
 * Replaces @ref src_str with @ref replace_str in
 * all instances matched by @ref regex.
 */
void
string_replace_regex (char ** str, const char * regex, const char * replace_str);

char *
string_replace (const char * str, const char * from, const char * to);

/**
 * Gets the string in the given regex group.
 *
 * @return A newly allocated string or NULL.
 */
char *
string_get_regex_group (const char * str, const char * regex, int group);

/**
 * Gets the string in the given regex group as an
 * integer.
 *
 * @param def Default.
 *
 * @return The int, or default.
 */
int
string_get_regex_group_as_int (
  const char * str,
  const char * regex,
  int          group,
  int          def);

/**
 * Returns the integer found at the end of a string like "My String 3" -> 3, or
 * -1 if no number is found.
 *
 * See https://www.debuggex.com/cheatsheet/regex/pcre for more info.
 *
 * @return The integer found at the end of the string, or -1 if no number is
 * found, and the string without the number (including the space).
 */
std::pair<int, std::string>
string_get_int_after_last_space (const std::string &str);

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
string_array_sort_and_remove_duplicates (char ** str_arr);

/**
 * Copies the string src to the buffer in @ref dest after reallocating the
 * buffer in @ref dest to the length of @ref src.
 *
 * If @ref src is nullptr, the string at @ref dest is free'd and the pointer is
 * set to NULL.
 */
void
string_copy_w_realloc (char ** dest, const char * src);

/**
 * Returns a new string with only ASCII alphanumeric
 * characters and replaces the rest with underscore.
 */
char *
string_symbolify (const char * in);

/**
 * Returns whether the string is NULL or empty.
 */
bool
string_is_empty (const char * str);

/**
 * Compares two UTF-8 strings using approximate case-insensitive ordering.
 *
 * @return < 0 if @param s1 compares before @param s2, 0 if they compare equal,
 *          > 0 if @param s1 compares after @param s2
 *
 * @note Taken from src/libedataserver/e-data-server-util.c in
 *evolution-data-center (e_util_utf8_strcasecmp).
 **/
int
string_utf8_strcasecmp (const char * s1, const char * s2);

/**
 * Expands environment variables enclosed in ${} in the given string.
 */
char *
string_expand_env_vars (const char * src);

void
string_print_strv (const char * prefix, char ** strv);

/**
 * Clones the given string array.
 */
char **
string_array_clone (const char ** src);

#include <cstdlib>
#include <utility>

class CStringRAII
{
public:
  // Constructor that takes ownership of the given string
  explicit CStringRAII (char * str) noexcept : str_ (str) { }

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

private:
  char * str_;
};

std::string
string_join (const std::vector<std::string> &strings, std::string_view delimiter);

/**
 * @}
 */

#endif
