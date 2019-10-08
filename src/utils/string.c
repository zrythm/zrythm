/*
 * utils/string.h - string utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <string.h>

#include "utils/arrays.h"
#include "utils/string.h"

#include <gtk/gtk.h>

/**
 * Returns if the string is ASCII.
 */
int
string_is_ascii (const char * string)
{
  unsigned long i;
  if (!string || strlen (string) == 0)
    return 0;
  for (i = 0; i < strlen (string); i++)
    {
      if (!string_char_is_ascii (string[i]))
        {
          return 0;
        }
    }
  return 1;
}

/**
 * Returns a new string up to the character before
 * the first non-ascii character, or until the
 * end.
 */
char *
string_stop_at_first_non_ascii (
  const char * str)
{
  for (int i = 0; i < (int) strlen (str); i++)
    {
      if (!string_char_is_ascii (str[i]))
        return
          g_strdup_printf (
            "%.*s", i, str);
    }
  return g_strdup (str);
}

/**
 * Returns the matched string if the string array
 * contains the given substring.
 */
char *
string_array_contains_substr (
  char ** str_array,
  int     num_str,
  const char *  substr)
{
  for (int i = 0; i < num_str; i++)
    {
      if (g_str_match_string (
        substr,
        str_array[i],
        0))
        return str_array[i];
    }

  return NULL;
}

/**
 * Returns if the two strings are equal.
 */
int
string_is_equal (
  const char * str1,
  const char * str2,
  int          ignore_case)
{
  if (ignore_case)
    {
      char * str1_casefolded =
        g_utf8_casefold (
          str1, -1);
      char * str2_casefolded =
        g_utf8_casefold (
          str2, -1);
      int ret =
        !g_strcmp0 (
          str1_casefolded,
          str2_casefolded);
      g_free (str1_casefolded);
      g_free (str2_casefolded);
      return ret;
    }
  else
    {
      return !g_strcmp0 (str1, str2);
    }
}

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
  const int    accept_alternatives)
{
  return
    g_str_match_string (
      substr,
      str,
      accept_alternatives);
}

int
string_ends_with (
  const char * str,
  const char * end_str,
  const int    ignore_case)
{
  /* TODO */
  g_return_val_if_reached (-1);
}

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI Region #1" -> "MIDI_Region_1".
 */
char *
string_convert_to_filename (
  const char * str)
{
  /* convert illegal characters to '_' */
  char * new_str = g_strdup (str);
  for (int i = 0; i < (int) strlen (str); i++)
    {
      if (str[i] == '#' ||
          str[i] == '%' ||
          str[i] == '&' ||
          str[i] == '{' ||
          str[i] == '}' ||
          str[i] == '\\' ||
          str[i] == '<' ||
          str[i] == '>' ||
          str[i] == '*' ||
          str[i] == '?' ||
          str[i] == '/' ||
          str[i] == ' ' ||
          str[i] == '$' ||
          str[i] == '!' ||
          str[i] == '\'' ||
          str[i] == '"' ||
          str[i] == ':' ||
          str[i] == '@')
        new_str[i] = '_';
    }
  return new_str;
}

/**
 * Removes non-ascii characters from the given
 * string.
 */
void
string_remove_non_ascii_chars (
  char * str)
{
  int count, len;
  for (len = 0; str[len] != '\0'; ++len);
  g_warn_if_fail (len > 0);

  count = len - 1;
  while (count >= 0 && str[count] != 0)
    {
      char chr = str[count];
      if (chr < 32 ||
          chr > 126)
        {
          array_delete (
            str, len, chr);
          len--;
        }
      count--;
    }
}

/**
 * Removes occurrences of the given character
 * from the string.
 */
void
string_remove_char (
  char *     str,
  const char remove)
{
  int count, len;
  for (len = 0; str[len] != '\0'; ++len);
  g_warn_if_fail (len > 0);

  count = len - 1;
  while (count >= 0 && str[count] != 0)
    {
      char chr = str[count];
      if (chr == remove)
        {
          array_delete (
            str, len, chr);
          len--;
        }
      count--;
    }
}

/**
 * Returns the index of the first occurrence of
 * the search char, or -1 if not found.
 */
int
string_index_of_char (
  const char * str,
  const char   search_char)
{
  int len = (int) strlen (str);
  for (int i = 0; i < len; i++)
    {
      if (str[i] == search_char)
        return i;
    }

  return -1;
}

/**
 * Removes any bak, bak1 etc suffixes from the
 * string and returns a newly allocated string.
 */
char *
string_get_substr_before_backup_ext (
  const char * str)
{
  /* get the part without .bak */
  char ** parts =
    g_strsplit (
      str, ".bak", 0);
  char * part = g_strdup (parts[0]);
  g_strfreev (parts);
  return part;
}
