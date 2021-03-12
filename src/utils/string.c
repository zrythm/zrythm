/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/objects.h"
#include "utils/string.h"

#include <gtk/gtk.h>

#include <pcre.h>
#include <regex.h>

int
string_is_ascii (const char * string)
{
  return g_str_is_ascii (string);
#if 0
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
#endif
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
 * Returns if the two strings are exactly equal.
 */
bool
string_is_equal (
  const char * str1,
  const char * str2)
{
  return !g_strcmp0 (str1, str2);
}

/**
 * Returns if the two strings are equal ignoring
 * case.
 */
bool
string_is_equal_ignore_case (
  const char * str1,
  const char * str2)
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

/**
 * Returns if the given string contains the given
 * substring.
 */
bool
string_contains_substr (
  const char * str,
  const char * substr)
{
  return g_strrstr (str, substr) != NULL;
}

bool
string_contains_substr_case_insensitive (
  const char * str,
  const char * substr)
{
  char new_str[strlen (str) + 1];
  new_str[strlen (str)] = '\0';
  string_to_upper (str, new_str);
  char new_substr[strlen (substr) + 1];
  new_substr[strlen (substr)] = '\0';
  string_to_upper (substr, new_substr);

  return
    string_contains_substr (new_str, new_substr);
}

/**
 * Converts the given string to uppercase in \ref
 * out.
 *
 * Assumes \ref out is already allocated to as many
 * chars as \ref in.
 */
void
string_to_upper (
  const char * in,
  char *       out)
{
  const char * src = in;
  char * dest = out;
  while (*src)
    {
      *dest = g_ascii_toupper (*src);
      src++;
      dest++;
    }
}

/**
 * Returns a newly allocated string that is a
 * filename version of the given string.
 *
 * Example: "MIDI ZRegion #1" -> "MIDI_Region_1".
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
 * Removes the suffix starting from \ref suffix
 * from \ref full_str and returns a newly allocated
 * string.
 */
char *
string_get_substr_before_suffix (
  const char * str,
  const char * suffix)
{
  g_return_val_if_fail (str, NULL);

  /* get the part without the suffix */
  char ** parts =
    g_strsplit (
      str, suffix, 0);
  char * part = g_strdup (parts[0]);
  g_strfreev (parts);
  return part;
}

/**
 * Removes everything up to and including the first
 * match of \ref match from the start of the string
 * and returns a newly allocated string.
 */
char *
string_remove_until_after_first_match (
  const char * str,
  const char * match)
{
  g_return_val_if_fail (str, NULL);

  char ** parts =
    g_strsplit (
      str, match, 2);
#if 0
  g_message ("after removing prefix: %s", prefix);
  g_message ("part 0 %s", parts[0]);
  g_message ("part 1 %s", parts[1]);
#endif
  char * part = g_strdup (parts[1]);
  g_strfreev (parts);
  return part;
}

/**
 * Replaces @ref str with @ref replace_str in
 * all instances matched by @ref regex.
 */
void
string_replace_regex (
  char **      str,
  const char * regex,
  const char * replace_str)
{
  /* edited from https://stackoverflow.com/questions/8044081/how-to-do-regex-string-replacements-in-pure-c */
  regex_t reg;

  /* if regex can't commpile pattern, do nothing */
  if (regcomp (&reg, regex, REG_EXTENDED))
    {
      g_warning (
        "Could not compile regex: %s", regex);
    }

  /* return if no matches */
  if (reg.re_nsub == 0)
    {
      g_debug ("no matches");
      return;
    }

  /* count back references in replace_str */
  int br = 0;
  const char * p = replace_str;
  while (1)
    {
      /* skip non-control chars */
      while (*p > 31 || *p > 8)
        {
          p++;
        }

      if (*p)
        {
          br++;
          p++;
        }
      else
        {
          break;
        }
    }
  /*g_debug ("num back references: %d", br);*/

  size_t nmatch = reg.re_nsub;
  regmatch_t m[nmatch + 1];
  memset (
    &m, 0, sizeof (regmatch_t) * (nmatch + 1));

  /* look for matches and replace */
  while (!regexec (
            &reg, *str, nmatch + 1, m, 0))
    {
      /*g_debug ("match");*/

      /* make enough room */
      char * new =
        object_new_n (
          strlen (*str) +
            (size_t) (m[0].rm_eo - m[0].rm_so) + 1,
          char);
      new[0] = '\0';

      const char * rpl = replace_str;
      p = replace_str;

      /* append the full match */
      strncat (new, *str, (size_t) m[0].rm_so);

      /* go through each back reference */
      for (int k = 0; k < br; k++)
        {
          /* skip printable char */
          while (*p > 16)
            {
              p++;
            }

          /* back reference (e.g., \1, \2,
           * ...) */
          int c = *p;

          /* add head of rpl */
          strncat (new, rpl, (size_t) (p - rpl));

          /* concat match */
          strncat (
            new, *str + m[c].rm_so,
            (size_t) (m[c].rm_eo - m[c].rm_so));

          /* skip back reference, next match */
          rpl = p++;
        }

      /* trailing of rpl */
      strcat (new, p);

      /* trailing text in *str */
      strcat (new, *str + m[0].rm_eo);
      free (*str);
      *str = strdup (new);
      free (new);
    }
  /* ajust size */
  *str = g_realloc (*str, strlen (*str) + 1);
}

char *
string_replace (
  const char * str,
  const char * from,
  const char * to)
{
  char ** split =
    g_strsplit (str, from, -1);
  char * new_str =
    g_strjoinv (to, split);
  g_strfreev (split);
  return new_str;
}

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
  int          def)
{
  char * res =
    string_get_regex_group (str, regex, group);
  if (res)
    {
      int res_int = atoi (res);
      g_free (res);
      return res_int;
    }
  else
    return def;
}

/**
 * Gets the string in the given regex group.
 *
 * @return A newly allocated string or NULL.
 */
char *
string_get_regex_group (
  const char * str,
  const char * regex,
  int          group)
{
  int OVECCOUNT = 60;
  const char *error;
  int   erroffset;
  pcre *re;
  int   rc;
  int   ovector[OVECCOUNT];

  re =
    pcre_compile (
      /* pattern */
      regex,
      /* options */
      0,
      /* error message */
      &error,
      /* error offset */
      &erroffset,
      /* use default character tables */
      0);

  if (!re)
    {
      g_warning (
        "pcre_compile failed (offset: %d), %s",
        erroffset, error);
      return NULL;
    }

  rc =
    pcre_exec (
      re, /* the compiled pattern */
      0,  /* no extra data - pattern was not studied */
      str, /* the string to match */
      strlen (str), /* the length of the string */
      0,   /* start at offset 0 in the subject */
      0,   /* default options */
      ovector, /* output vector for substring information */
      OVECCOUNT); /* number of elements in the output vector */

  if (rc < 0)
    {
      switch (rc)
        {
        case PCRE_ERROR_NOMATCH:
          /*g_message ("String %s didn't match", str);*/
          break;

        default:
          g_message (
            "Error while matching \"%s\": %d",
            str, rc);
          break;
        }
      free (re);
      return NULL;
    }

#if 0
  for (int i = 0; i < rc; i++)
    {
      g_message (
        "%2d: %.*s", i,
        ovector[2 * i + 1] - ovector[2 * i],
        str + ovector[2 * i]);
    }
#endif

  return
    g_strdup_printf (
      "%.*s",
      ovector[2 * group + 1] - ovector[2 * group],
      str + ovector[2 * group]);
}

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
  char *       str_without_num)
{
  int OVECCOUNT = 60;
  const char *error;
  int   erroffset;
  pcre *re;
  int   rc;
  int   i;
  int   ovector[OVECCOUNT];

  const char *regex = "(.*) ([\\d]+)";

  re =
    pcre_compile (
      /* pattern */
      regex,
      /* options */
      0,
      /* error message */
      &error,
      /* error offset */
      &erroffset,
      /* use default character tables */
      0);

  if (!re)
    {
      g_error (
        "pcre_compile failed (offset: %d), %s",
        erroffset, error);
      return -1;
    }

  rc =
    pcre_exec (
      re, /* the compiled pattern */
      0,  /* no extra data - pattern was not studied */
      str, /* the string to match */
      strlen(str), /* the length of the string */
      0,   /* start at offset 0 in the subject */
      0,   /* default options */
      ovector, /* output vector for substring information */
      OVECCOUNT); /* number of elements in the output vector */

  if (rc < 0)
    {
      switch (rc)
        {
        case PCRE_ERROR_NOMATCH:
          g_message (
            "%s: String %s didn't match",
            __func__, str);
          break;

        default:
          g_message (
            "%s: Error while matching \"%s\": %d",
            __func__, str, rc);
          break;
        }
      free (re);
      return -1;
    }

#if 0
  for (i = 0; i < rc; i++)
    {
      g_message (
        "%2d: %.*s", i,
        ovector[2 * i + 1] - ovector[2 * i],
        str + ovector[2 * i]);
    }
#endif

  if (str_without_num)
    {
      i = rc - 2;
      sprintf (
        str_without_num, "%.*s",
        ovector[2 * i + 1] - ovector[2 * i],
        str + ovector[2 * i]);
    }

  i = rc - 1;
  return atoi (str + ovector[i * 2]);
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
string_array_sort_and_remove_duplicates (
  char ** str_arr)
{
  /* TODO */
  g_return_val_if_reached (NULL);
}

/**
 * Copies the string src to the buffer in \ref dest
 * after reallocating the buffer in \ref dest to
 * the length of \ref src.
 *
 * If \ref src is NULL, the string at \ref dest is
 * free'd and the pointer is set to NULL.
 */
void
string_copy_w_realloc (
  char **      dest,
  const char * src)
{
  g_return_if_fail (
    dest &&
    ((!*dest && !src) || (*dest != src)));
  if (src)
    {
      size_t strlen_src = strlen (src);
      *dest =
        g_realloc (
          *dest,
          (strlen_src + 1) * sizeof (char));
      strcpy (*dest, src);
    }
  else
    {
      if (*dest)
        {
          free (*dest);
          *dest = NULL;
        }
    }
}
