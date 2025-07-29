// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cstring>
#include <regex>

#include "utils/logger.h"
#include "utils/string.h"

namespace zrythm::utils
{

Utf8String
Utf8String::from_juce_string (const juce::String &str)
{
  Utf8String ret;
  ret.str_ = str.toUTF8 ();
  return ret;
}

juce::String
Utf8String::to_juce_string () const
{
  return juce::String::fromUTF8 (str_.data ());
}

juce::File
Utf8String::to_juce_file () const
{
  return { to_juce_string () };
}

Utf8String
Utf8String::escape_html () const
{
  return from_qstring (to_qstring ().toHtmlEscaped ());
}

bool
Utf8String::is_ascii () const
{
  return std::ranges::all_of (str_, [] (unsigned char c) { return c <= 127; });
}

bool
Utf8String::contains_substr (const Utf8String &substr) const
{
  return to_qstring ().contains (substr.to_qstring (), Qt::CaseSensitive);
}

bool
Utf8String::contains_substr_case_insensitive (const Utf8String &substr) const
{
  return to_qstring ().contains (substr.to_qstring (), Qt::CaseInsensitive);
}

bool
Utf8String::is_equal_ignore_case (const Utf8String &other) const
{
  return to_qstring ().compare (other.to_qstring (), Qt::CaseInsensitive) == 0;
}

Utf8String
Utf8String::to_upper () const
{
  return from_qstring (to_qstring ().toUpper ());
}

Utf8String
Utf8String::to_lower () const
{
  return from_qstring (to_qstring ().toLower ());
}

Utf8String
Utf8String::convert_to_filename () const
{ /* convert illegal characters to '_' */
  return from_utf8_encoded_string (
    std::regex_replace (str_, std::regex (R"([#%&{}\\<>*?/$!'":@+`|= ])"), "_"));
}

Utf8String
Utf8String::get_substr_before_suffix (const Utf8String &suffix) const
{
  /* get the part without the suffix */
  auto qstr = to_qstring ();
  auto idx = qstr.indexOf (suffix.to_qstring ());
  if (idx == -1)
    {
      return *this;
    }
  return from_qstring (qstr.left (idx));
}

Utf8String
Utf8String::remove_until_after_first_match (const Utf8String &match) const
{
  if (empty ())
    return {};

  auto pos = str_.find (match.str_);
  if (pos == std::string::npos)
    return *this;

  return from_utf8_encoded_string (str_.substr (pos + match.str_.length ()));
}

Utf8String
Utf8String::replace (const Utf8String &from, const Utf8String &to) const
{
  std::string result = str_;
  size_t      start_pos = 0;
  while ((start_pos = result.find (from.str_, start_pos)) != std::string::npos)
    {
      result.replace (start_pos, from.str_.length (), to.str_);
      start_pos += to.str_.length (); // Move to the end of the replaced string
    }
  return from_utf8_encoded_string (result);
}

int
Utf8String::get_regex_group_as_int (const Utf8String &regex, int group, int def)
  const
{
  auto res = get_regex_group (regex, group);
  if (!res.empty ())
    {
      int res_int = atoi (res.c_str ());
      return res_int;
    }

  return def;
}

Utf8String
Utf8String::get_regex_group (const Utf8String &regex, int group) const
{
  std::regex  re (regex.str_);
  std::cmatch match;
  if (
    std::regex_search (c_str (), match, re)
    && group < static_cast<decltype (group)> (match.size ()))
    {
      return from_utf8_encoded_string (
        match[static_cast<size_t> (group)].str ());
    }

  z_warning ("not found: regex '{}', str '{}'", regex, str_);
  return {};
}

std::pair<int, Utf8String>
Utf8String::get_int_after_last_space () const
{
  std::regex  re (R"((.*) (\d+))");
  std::smatch match;
  if (std::regex_match (str_, match, re))
    {
      return {
        std::stoi (match[2]), from_utf8_encoded_string (match[1].str ())
      };
    }
  return { -1, {} };
}

Utf8String
Utf8String::symbolify () const
{
  std::string out;
  out.reserve (str_.length ());

  for (const auto &c : str_)
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

  return from_utf8_encoded_string (out);
}

Utf8String
Utf8String::expand_env_vars () const
{
  std::regex  env_var_regex (R"(\$\{([^}]+)\})");
  auto        result = *this;
  std::smatch match;

  while (std::regex_search (result.str (), match, env_var_regex))
    {
      std::string  env_var = match[1].str ();
      const char * env_val = std::getenv (env_var.c_str ());
      std::string  replacement = env_val ? env_val : "";
      result = result.replace (
        Utf8String::from_utf8_encoded_string (match[0].str ()),
        Utf8String::from_utf8_encoded_string (replacement));
    }

  return result;
}

} // namespace zrythm::utils
