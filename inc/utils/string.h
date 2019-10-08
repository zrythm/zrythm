/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * String utilities.
 */

#ifndef __UTILS_STRING_H__
#define __UTILS_STRING_H__

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns if the character is ASCII.
 */
static inline int
string_char_is_ascii (
  const char character)
{
  return character >= 32 && character <= 126;
}

/**
 * Returns if the string is ASCII.
 */
int
string_is_ascii (
  const char * string);

/**
 * Returns a new string up to the character before
 * the first non-ascii character, or until the
 * end.
 */
char *
string_stop_at_first_non_ascii (
  const char * str);

/**
 * Returns the matched string if the string array
 * contains the given substring.
 */
char *
string_array_contains_substr (
  char ** str_array,
  int     num_str,
  const char *  substr);

/**
 * Returns if the given string contains the given
 * substring.
 *
 * @param accept_alternatives Accept ASCII
 *   alternatives.
 */
int
string_contains_substr (
  const char * str,
  const char * substr,
  const int    accept_alternatives);

int
string_ends_with (
  const char * str,
  const char * end_str,
  const int    ignore_case);

/**
 * Removes non-ascii characters from the given
 * string.
 */
void
string_remove_non_ascii_chars (
  char * str);

/**
 * Removes occurrences of the given character
 * from the string.
 */
void
string_remove_char (
  char *     str,
  const char remove);

/**
 * Returns if the two strings are equal.
 */
int
string_is_equal (
  const char * str1,
  const char * str2,
  int          ignore_case);

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI Region #1" -> "MIDI_Region_1".
 */
char *
string_convert_to_filename (
  const char * str);

/**
 * Returns the index of the first occurrence of
 * the search char, or -1 if not found.
 */
int
string_index_of_char (
  const char * str,
  const char   search_char);

/**
 * Removes any bak, bak1 etc suffixes from the
 * string and returns a newly allocated string.
 */
char *
string_get_substr_before_backup_ext (
  const char * str);

/**
 * @}
 */

#endif
