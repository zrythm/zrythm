// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdlib>
#include <string>
#include <utility>

#include "utils/traits.h"

#include <QHash>
#include <QString>
#include <QUrl>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace juce
{
class String;
class File;
}

/**
 * @brief String utilities.
 */
namespace zrythm::utils
{

/**
 * @brief Lightweight UTF-8 string wrapper with safe conversions.
 *
 * Guarantees valid UTF-8 storage and explicit encoding handling.
 */
class Utf8String
{
public:
  constexpr Utf8String () noexcept = default;
  constexpr Utf8String (const char8_t * str)
      : Utf8String (std::u8string_view{ str })
  {
  }
  constexpr Utf8String (std::u8string_view str)
      : str_ (str.begin (), str.end ())
  {
  }

  Utf8String (const Utf8String &) = default;
  Utf8String (Utf8String &&) = default;
  Utf8String &operator= (const Utf8String &) = default;
  Utf8String &operator= (Utf8String &&) = default;
  ~Utf8String () = default;

  static Utf8String from_path (const fs::path &path)
  {
    return Utf8String{ path.generic_u8string () };
  }
  static Utf8String from_qstring (const QString &str)
  {
    Utf8String ret;
    ret.str_ = str.toUtf8 ().toStdString ();
    return ret;
  }
  static Utf8String from_qurl (const QUrl &url)
  {
    return from_qstring (url.toLocalFile ());
  }
  static Utf8String from_juce_string (const juce::String &str);

  /**
   * @brief Construct from a std::string_view that we are 100% sure is
   * UTF8-encoded.
   *
   * @warning This is not checked, so use with caution.
   */
  static constexpr Utf8String from_utf8_encoded_string (std::string_view str)
  {
    Utf8String ret;
    ret.str_ = str;
    return ret;
  }

  // --- Accessors ---
  std::string_view   view () const noexcept { return str_; }
  const std::string &str () const noexcept { return str_; }
  const char *       c_str () const noexcept { return str_.c_str (); }
  auto               empty () const noexcept { return str_.empty (); }

  // --- Conversions ---
  fs::path      to_path () const { return { to_u8_string () }; }
  std::u8string to_u8_string () const { return { str_.begin (), str_.end () }; }
  QString       to_qstring () const { return QString::fromUtf8 (c_str ()); }
  juce::String  to_juce_string () const;
  juce::File    to_juce_file () const;

  // --- Operators ---
  explicit operator std::string_view () const noexcept { return view (); }
  bool     operator== (std::string_view other) const noexcept
  {
    return view () == other;
  }
  Utf8String &operator+= (const Utf8String &other)
  {
    str_ += other.str_;
    return *this;
  }
  Utf8String operator+ (const Utf8String &other) const
  {
    return Utf8String::from_utf8_encoded_string (str_ + other.str_);
  }

  operator fs::path () const { return to_path (); }
  operator QString () const { return to_qstring (); }
  friend std::ostream &operator<< (std::ostream &os, const Utf8String &str)
  {
    return os << str.view ();
  }

  // --- Utils ---
  Utf8String escape_html () const;
  bool       is_ascii () const;
  bool       contains_substr (const Utf8String &substr) const;
  bool       contains_substr_case_insensitive (const Utf8String &substr) const;
  bool       is_equal_ignore_case (const Utf8String &other) const;
  Utf8String to_upper () const;
  Utf8String to_lower () const;

  /**
   * Returns a filename-safe version of the given string.
   *
   * Example: "MIDI Region #1" -> "MIDI_Region_1".
   */
  Utf8String convert_to_filename () const;

  /**
   * Removes the suffix starting from @ref suffix
   * from @ref full_str and returns a newly allocated
   * string.
   */
  Utf8String get_substr_before_suffix (const Utf8String &suffix) const;

  /**
   * Removes everything up to and including the first match of @ref match from
   * the start of the string.
   */
  Utf8String remove_until_after_first_match (const Utf8String &match) const;

  Utf8String replace (const Utf8String &from, const Utf8String &to) const;

  /**
   * Gets the string in the given regex group.
   *
   * @return The string, or an empty string if nothing found.
   */
  Utf8String get_regex_group (const Utf8String &regex, int group) const;

  /**
   * Gets the string in the given regex group as an integer.
   *
   * @param def Default.
   *
   * @return The int, or default.
   */
  int
  get_regex_group_as_int (const Utf8String &regex, int group, int def) const;

  /**
   * Returns the integer found at the end of a string like "My String 3" -> 3,
   * or -1 if no number is found.
   *
   * See https://www.debuggex.com/cheatsheet/regex/pcre for more info.
   *
   * @return The integer found at the end of the string, or -1 if no number is
   * found, and the string without the number (including the space).
   */
  std::pair<int, Utf8String> get_int_after_last_space () const;

  /**
   * Returns a new string with only ASCII alphanumeric characters and replaces
   * the rest with underscore.
   */
  Utf8String symbolify () const;

  /**
   * Expands environment variables enclosed in ${} in the given string.
   */
  Utf8String expand_env_vars () const;

  static Utf8String
  join (const RangeOf<Utf8String> auto &strings, const Utf8String &delimiter)
  {
    return Utf8String::from_utf8_encoded_string (fmt::format (
      "{}",
      fmt::join (
        std::views::transform (strings, &Utf8String::view), delimiter.view ())));
  }

  // --- Comparisons ---
  friend auto operator<=> (const Utf8String &a, const Utf8String &b) noexcept
  {
    return a.str_ <=> b.str_;
  }
  friend bool operator== (const Utf8String &a, const Utf8String &b) noexcept
  {
    return a.str_ == b.str_;
  }

private:
  std::string str_;
};

/**
 * @brief C string RAII wrapper.
 *
 * @note UTF8 assumed.
 */
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

  Utf8String to_utf8_string () const
  {
    return Utf8String::from_utf8_encoded_string (str_);
  }

private:
  char * str_;
};

}; // namespace zrythm::utils

// Formatter for Utf8String
template <>
struct fmt::formatter<zrythm::utils::Utf8String>
    : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const zrythm::utils::Utf8String &s, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (s.view (), ctx);
  }
};

// Hasher for Utf8String
static inline size_t
qHash (const zrythm::utils::Utf8String &t, size_t seed = 0) noexcept
{
  return qHash (t.view (), seed); // <-- qHash used as public API
}
