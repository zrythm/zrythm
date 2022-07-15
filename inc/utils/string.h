/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdbool.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Returns if the string is ASCII.
 */
int
string_is_ascii (const char * string);

/**
 * Returns the matched string if the string array
 * contains the given substring.
 */
char *
string_array_contains_substr (
  char **      str_array,
  int          num_str,
  const char * substr);

/**
 * Returns if the given string contains the given
 * substring.
 */
bool
string_contains_substr (const char * str, const char * substr);

bool
string_contains_substr_case_insensitive (
  const char * str,
  const char * substr);

/**
 * Converts the given string to uppercase in \ref
 * out.
 *
 * Assumes \ref out is already allocated to as many
 * chars as \ref in.
 */
void
string_to_upper (const char * in, char * out);

/**
 * Converts the given string to lowercase in \ref
 * out.
 *
 * Assumes \ref out is already allocated to as many
 * chars as \ref in.
 */
void
string_to_lower (const char * in, char * out);

/**
 * Returns if the two strings are exactly equal.
 */
#define string_is_equal(str1, str2) (!g_strcmp0 (str1, str2))

/**
 * Returns if the two strings are equal ignoring
 * case.
 */
bool
string_is_equal_ignore_case (
  const char * str1,
  const char * str2);

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI ZRegion #1" -> "MIDI_Region_1".
 */
char *
string_convert_to_filename (const char * str);

/**
 * Removes the suffix starting from \ref suffix
 * from \ref full_str and returns a newly allocated
 * string.
 */
MALLOC
NONNULL
char *
string_get_substr_before_suffix (
  const char * str,
  const char * suffix);

/**
 * Removes everything up to and including the first
 * match of \ref match from the start of the string
 * and returns a newly allocated string.
 */
char *
string_remove_until_after_first_match (
  const char * str,
  const char * match);

/**
 * Replaces @ref src_str with @ref replace_str in
 * all instances matched by @ref regex.
 */
void
string_replace_regex (
  char **      str,
  const char * regex,
  const char * replace_str);

char *
string_replace (
  const char * str,
  const char * from,
  const char * to);

/**
 * Gets the string in the given regex group.
 *
 * @return A newly allocated string or NULL.
 */
char *
string_get_regex_group (
  const char * str,
  const char * regex,
  int          group);

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
 * Returns the integer found at the end of a string
 * like "My String 3" -> 3, or -1 if no number is
 * found.
 *
 * See https://www.debuggex.com/cheatsheet/regex/pcre
 * for more info.
 *
 * @param str_without_num A buffer to save the
 *   string without the number (including the space).
 */
int
string_get_int_after_last_space (
  const char * str,
  char *       str_without_num);

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
 * Copies the string src to the buffer in \ref dest
 * after reallocating the buffer in \ref dest to
 * the length of \ref src.
 *
 * If \ref src is NULL, the string at \ref dest is
 * free'd and the pointer is set to NULL.
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
PURE bool
string_is_empty (const char * str);

/**
 * @}
 */

#endif
