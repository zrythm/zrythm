// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cstring>
#include <regex>

#include "utils/logger.h"
#include "utils/string.h"

namespace zrythm::utils
{

namespace string
{

std::string
escape_html (const std::string &str)
{
  return qstring_to_std_string (std_string_to_qstring (str).toHtmlEscaped ());
}

juce::String
string_view_to_juce_string (std::string_view sv)
{
  juce::String juce_str = juce::String::createStringFromData (
    sv.data (), static_cast<int> (sv.size ()));
  return juce_str;
}

bool
is_ascii (std::string_view string)
{
  return std::ranges::all_of (string, [] (unsigned char c) {
    return c <= 127;
  });
}

bool
is_equal_ignore_case (const std::string &str1, const std::string &str2)
{
  return std::ranges::equal (str1, str2, [] (char a, char b) {
    return std::tolower (a) == std::tolower (b);
  });
}

bool
contains_substr (std::string_view str, std::string_view substr)
{
  return utils::std_string_to_qstring (std::string (str))
    .contains (
      utils::std_string_to_qstring (std::string{ substr }), Qt::CaseSensitive);
}

bool
contains_substr_case_insensitive (std::string_view str, std::string_view substr)
{
  return utils::std_string_to_qstring (std::string{ str })
    .contains (
      utils::std_string_to_qstring (std::string{ substr }), Qt::CaseInsensitive);
}

void
to_upper_ascii (std::string &str)
{
  std::ranges::transform (str, str.begin (), [] (unsigned char c) {
    return std::toupper (c);
  });
}

void
to_lower_ascii (std::string &str)
{
  std::ranges::transform (str, str.begin (), [] (unsigned char c) {
    return std::tolower (c);
  });
}

std::string
convert_to_filename (const std::string &str)
{
  /* convert illegal characters to '_' */
  return std::regex_replace (
    str, std::regex (R"([#%&{}\\<>*?/$!'":@+`|= ])"), "_");
}

std::string
get_substr_before_suffix (const std::string &str, const std::string &suffix)
{
  /* get the part without the suffix */
  auto qstr = utils::std_string_to_qstring (str);
  auto idx = qstr.indexOf (utils::std_string_to_qstring (suffix));
  if (idx == -1)
    {
      return str;
    }
  return qstring_to_std_string (qstr.left (idx));
}

std::string
remove_until_after_first_match (const std::string &str, const std::string &match)
{
  if (str.empty ())
    return "";

  auto pos = str.find (match);
  if (pos == std::string::npos)
    return str;

  return str.substr (pos + match.length ());
}

std::string
replace (const std::string &str, const std::string &from, const std::string &to)
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
get_regex_group_as_int (
  const std::string &str,
  const std::string &regex,
  int                group,
  int                def)
{
  auto res = get_regex_group (str, regex, group);
  if (!res.empty ())
    {
      int res_int = atoi (res.c_str ());
      return res_int;
    }

  return def;
}

std::string
get_regex_group (const std::string &str, const std::string &regex, int group)
{
  std::regex  re (regex);
  std::cmatch match;
  if (
    std::regex_search (str.c_str (), match, re)
    && group < static_cast<decltype (group)> (match.size ()))
    {
      return match[static_cast<size_t> (group)].str ();
    }

  z_warning ("not found: regex '{}', str '{}'", regex, str);
  return "";
}

std::pair<int, std::string>
get_int_after_last_space (const std::string &str)
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
array_sort_and_remove_duplicates (char ** str_arr)
{
  /* TODO */
  z_return_val_if_reached (nullptr);
}

std::string
symbolify (const std::string &in)
{
  std::string out;
  out.reserve (in.length ());

  for (const auto &c : in)
    {
      if (std::isalnum (static_cast<unsigned char> (c)))
        {
          out.push_back (c);
        }
      else
        {
          out.push_back ('_');
        }
    }

  return out;
}

/**
 * Returns whether the string is NULL or empty.
 */
bool
is_empty (const char * str)
{
  return !str || strlen (str) == 0;
}

std::string
expand_env_vars (const std::string &src)
{
  std::regex  env_var_regex (R"(\$\{([^}]+)\})");
  std::string result = src;
  std::smatch match;

  while (std::regex_search (result, match, env_var_regex))
    {
      std::string  env_var = match[1].str ();
      const char * env_val = std::getenv (env_var.c_str ());
      std::string  replacement = env_val ? env_val : "";
      result = replace (result, match[0].str (), replacement);
    }

  return result;
}

std::string
join (const std::vector<std::string> &strings, std::string_view delimiter)
{
  return fmt::format ("{}", fmt::join (strings, delimiter));
}

QString
qurl_to_path_qstring (const QUrl &url)
{
  return url.toLocalFile ();
}

}; // namespace zrythm::utils::string

std::string
juce_string_to_std_string (const juce::String &str)
{
  return str.toStdString ();
}

QString
juce_string_to_qstring (const juce::String &str)
{
  return std_string_to_qstring (juce_string_to_std_string (str));
}

}; // namespace zrythm::utils
