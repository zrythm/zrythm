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

#include "utils/string.h"

#include <gtk/gtk.h>

int
string_is_ascii (const char * string)
{
  unsigned long i;
  if (!string || strlen (string) == 0)
    return 0;
  for (i = 0; i < strlen (string); i++)
    {
      if (string[i] < 32 ||
          string[i] > 126)
        {
          return 0;
        }
    }
  return 1;
}

/**
 * Returns the matched string if the string array
 * contains the given substring.
 */
char *
string_array_contains_substr (
  char ** str_array,
  int     num_str,
  char *  substr)
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
  for (int i = 0; i < strlen (str); i++)
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
