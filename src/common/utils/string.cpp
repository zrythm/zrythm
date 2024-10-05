// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
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

#include <cstring>
#include <regex>

#include "common/utils/logger.h"
#include "common/utils/objects.h"
#include "common/utils/string.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

Glib::ustring
string_view_to_ustring (std::string_view sv)
{
  Glib::ustring ustr (sv.data (), sv.data () + sv.size ());
  return ustr;
}

juce::String
string_view_to_juce_string (std::string_view sv)
{
  juce::String juce_str =
    juce::String::createStringFromData (sv.data (), sv.size ());
  return juce_str;
}

bool
string_is_ascii (std::string_view string)
{
  return string_view_to_ustring (string).is_ascii ();
}

/**
 * Returns the matched string if the string array
 * contains the given substring.
 */
char *
string_array_contains_substr (char ** str_array, int num_str, const char * substr)
{
  for (int i = 0; i < num_str; i++)
    {
      if (g_str_match_string (substr, str_array[i], 0))
        return str_array[i];
    }

  return NULL;
}

bool
string_is_equal_ignore_case (const char * str1, const char * str2)
{
  char * str1_casefolded = g_utf8_casefold (str1, -1);
  char * str2_casefolded = g_utf8_casefold (str2, -1);
  int    ret = !g_strcmp0 (str1_casefolded, str2_casefolded);
  g_free (str1_casefolded);
  g_free (str2_casefolded);

  return ret;
}

bool
string_is_equal_ignore_case (const std::string &str1, const std::string &str2)
{
  return std::ranges::equal (str1, str2, [] (char a, char b) {
    return std::tolower (a) == std::tolower (b);
  });
}

/**
 * Returns if the given string contains the given
 * substring.
 */
bool
string_contains_substr (const char * str, const char * substr)
{
  return g_strrstr (str, substr) != NULL;
}

bool
string_contains_substr_case_insensitive (const char * str, const char * substr)
{
  std::string new_str (str);
  string_to_upper_ascii (new_str);
  std::string new_substr (substr);
  string_to_upper_ascii (new_substr);

  return string_contains_substr (new_str.c_str (), new_substr.c_str ());
}

void
string_to_upper_ascii (std::string &str)
{
  std::transform (str.begin (), str.end (), str.begin (), [] (unsigned char c) {
    return std::toupper (c);
  });
}

void
string_to_lower_ascii (std::string &str)
{
  std::transform (str.begin (), str.end (), str.begin (), [] (unsigned char c) {
    return std::tolower (c);
  });
}

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI Region #1" -> "MIDI_Region_1".
 */
char *
string_convert_to_filename (const char * str)
{
  /* convert illegal characters to '_' */
  char * new_str = g_strdup (str);
  for (int i = 0; i < (int) strlen (str); i++)
    {
      if (
        str[i] == '#' || str[i] == '%' || str[i] == '&' || str[i] == '{'
        || str[i] == '}' || str[i] == '\\' || str[i] == '<' || str[i] == '>'
        || str[i] == '*' || str[i] == '?' || str[i] == '/' || str[i] == ' '
        || str[i] == '$' || str[i] == '!' || str[i] == '\'' || str[i] == '"'
        || str[i] == ':' || str[i] == '@')
        new_str[i] = '_';
    }
  return new_str;
}

/**
 * Removes the suffix starting from @ref suffix
 * from @ref full_str and returns a newly allocated
 * string.
 */
char *
string_get_substr_before_suffix (const char * str, const char * suffix)
{
  /* get the part without the suffix */
  char ** parts = g_strsplit (str, suffix, 0);
  char *  part = g_strdup (parts[0]);
  g_strfreev (parts);
  return part;
}

/**
 * Removes everything up to and including the first
 * match of @ref match from the start of the string
 * and returns a newly allocated string.
 */
char *
string_remove_until_after_first_match (const char * str, const char * match)
{
  z_return_val_if_fail (str, nullptr);

  char ** parts = g_strsplit (str, match, 2);
#if 0
  z_info ("after removing prefix: {}", prefix);
  z_info ("part 0 {}", parts[0]);
  z_info ("part 1 {}", parts[1]);
#endif
  char * part = g_strdup (parts[1]);
  g_strfreev (parts);
  return part;
}

std::string
string_replace (
  const std::string &str,
  const std::string &from,
  const std::string &to)
{
  std::string result = str;
  size_t      start_pos = 0;
  while ((start_pos = result.find (from, start_pos)) != std::string::npos)
    {
      result.replace (start_pos, from.length (), to);
      start_pos += to.length (); // Move to the end of the replaced string
    }
  return result;
}

int
string_get_regex_group_as_int (
  const std::string &str,
  const std::string &regex,
  int                group,
  int                def)
{
  auto res = string_get_regex_group (str, regex, group);
  if (!res.empty ())
    {
      int res_int = atoi (res.c_str ());
      return res_int;
    }

  return def;
}

std::string
string_get_regex_group (
  const std::string &str,
  const std::string &regex,
  int                group)
{
  std::regex  re (regex);
  std::cmatch match;
  if (
    std::regex_search (str.c_str (), match, re)
    && group < static_cast<decltype (group)> (match.size ()))
    {
      return match[group].str ();
    }

  z_warning ("not found: regex '{}', str '{}'", regex, str);
  return "";
}

std::pair<int, std::string>
string_get_int_after_last_space (const std::string &str)
{
  std::regex  re (R"((.*) (\d+))");
  std::smatch match;
  if (std::regex_match (str, match, re))
    {
      return { std::stoi (match[2]), match[1] };
    }
  return { -1, "" };
}

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
string_array_sort_and_remove_duplicates (char ** str_arr)
{
  /* TODO */
  z_return_val_if_reached (nullptr);
}

/**
 * Returns a new string with only ASCII alphanumeric
 * characters and replaces the rest with underscore.
 */
char *
string_symbolify (const char * in)
{
  const size_t len = strlen (in);
  char *       out = (char *) object_new_n_sizeof (len + 1, 1);
  for (size_t i = 0; i < len; ++i)
    {
      if (g_ascii_isalnum (in[i]))
        {
          out[i] = in[i];
        }
      else
        {
          out[i] = '_';
        }
    }
  return out;
}

/**
 * Returns whether the string is NULL or empty.
 */
bool
string_is_empty (const char * str)
{
  if (!str || strlen (str) == 0)
    return true;

  return false;
}

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
string_utf8_strcasecmp (const char * s1, const char * s2)
{
  gchar *folded_s1, *folded_s2;
  gint   retval;

  z_return_val_if_fail (s1 != NULL && s2 != nullptr, -1);

  if (strcmp (s1, s2) == 0)
    return 0;

  folded_s1 = g_utf8_casefold (s1, -1);
  folded_s2 = g_utf8_casefold (s2, -1);

  retval = g_utf8_collate (folded_s1, folded_s2);

  g_free (folded_s2);
  g_free (folded_s1);

  return retval;
}

std::string
string_expand_env_vars (const std::string &src)
{
  std::regex  env_var_regex (R"(\$\{([^}]+)\})");
  std::string result = src;
  std::smatch match;

  while (std::regex_search (result, match, env_var_regex))
    {
      std::string  env_var = match[1].str ();
      const char * env_val = std::getenv (env_var.c_str ());
      std::string  replacement = env_val ? env_val : "";
      result = string_replace (result, match[0].str (), replacement);
    }

  return result;
}

void
string_print_strv (const char * prefix, char ** strv)
{
  const char * cur = NULL;
  GString *    gstr = g_string_new (prefix);
  g_string_append (gstr, ":");
  for (int i = 0; (cur = strv[i]) != NULL; i++)
    {
      g_string_append_printf (gstr, " %s |", cur);
    }
  char * res = g_string_free (gstr, false);
  res[strlen (res) - 1] = '\0';
  z_info ("{}", res);
  g_free (res);
}

/**
 * Clones the given string array.
 */
char **
string_array_clone (const char ** src)
{
  GStrvBuilder * builder = g_strv_builder_new ();
  g_strv_builder_addv (builder, src);
  return g_strv_builder_end (builder);
}

std::string
string_join (const std::vector<std::string> &strings, std::string_view delimiter)
{
  return fmt::format ("{}", fmt::join (strings, delimiter));
}