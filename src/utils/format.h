// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <filesystem>

#include "utils/string.h"
#include "utils/traits.h"

#include <QObject>
#include <QString>

#include "juce_wrapper.h"
#include <boost/describe.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <magic_enum.hpp>

/**
 * @addtogroup utils
 *
 * @{
 */

using namespace zrythm;

#define DEFINE_FORMATTER_PRELUDE \
  bool                                            translate_ = false; \
  template <typename ParseContext> constexpr auto parse (ParseContext &ctx) \
  { \
    auto it = ctx.begin (); \
    translate_ = (it != ctx.end () && *it == 't'); \
    return it + translate_; \
  }

#define VA_ARGS_SIZE(...) \
  std::tuple_size<decltype (std::make_tuple (__VA_ARGS__))>::value

/**
 * @brief Defines a formatter for the given object type.
 *
 * This macro defines a formatter for the given object type `obj_type` using the
 * provided `formatter_func` function. The formatter allows the object to be
 * formatted using the `fmt` library, for example:
 *
 * obj_type obj;
 * fmt::format("{}", obj);
 *
 * The `formatter_func` should be a function that takes the object and returns a
 * string representation of it.
 *
 * Usage examples:
 *
 * 1. Using a lambda with a namespaced type:
 * DEFINE_OBJECT_FORMATTER(zrythm::dsp::Position, Position, [](const
 * zrythm::dsp::Position& obj) { return std::to_string(obj.get_ticks());
 * })
 *
 * 2. Using a named function:
 * std::string formatMyClass(const MyClass& obj) {
 *     return std::to_string(obj.getValue());
 * }
 * DEFINE_OBJECT_FORMATTER(MyClass, Myclass, formatMyClass)
 *
 * @param obj_type The object type to define the formatter for.
 * @param function_prefix The prefix to use for the generated helper functions.
 * @param formatter_func The function to use for formatting the object.
 */
#define DEFINE_OBJECT_FORMATTER(obj_type, function_prefix, formatter_func) \
  namespace fmt \
  { \
  template <> struct formatter<obj_type> \
  { \
    DEFINE_FORMATTER_PRELUDE \
\
    template <typename FormatContext> \
    auto format (const obj_type &val, FormatContext &ctx) const \
    { \
      return format_to (ctx.out (), FMT_STRING ("{}"), formatter_func (val)); \
    } \
  }; \
  } \
\
  inline utils::Utf8String function_prefix##_to_string (const obj_type &obj) \
  { \
    return utils::Utf8String::from_utf8_encoded_string ( \
      fmt::format ("{}", obj)); \
  } \
\
  inline void function_prefix##_print (const obj_type &obj) \
  { \
    z_debug ("{}", obj); \
  }

#define DEFINE_ENUM_FORMATTER(enum_type, enum_name, ...) \
  namespace fmt \
  { \
  template <> struct formatter<enum_type> \
  { \
    DEFINE_FORMATTER_PRELUDE \
\
    template <typename FormatContext> \
    auto format (const enum_type &val, FormatContext &ctx) const \
    { \
      static constexpr std::array<const char *, VA_ARGS_SIZE (__VA_ARGS__)> \
                   enum_strings = { __VA_ARGS__ }; \
      const char * str = enum_strings.at (static_cast<int> (val)); \
      return format_to ( \
        ctx.out (), FMT_STRING ("{}"), \
        translate_ \
          ? utils::Utf8String::from_qstring (QObject::tr (str)).c_str () \
          : str); \
    } \
  }; \
  } \
\
  inline const char * const * enum_name##_get_strings () \
  { \
    static constexpr std::array<const char *, VA_ARGS_SIZE (__VA_ARGS__)> \
      enum_strings = { __VA_ARGS__ }; \
    return enum_strings.data (); \
  } \
\
  inline utils::Utf8String enum_name##_to_string ( \
    enum_type val, bool translate = false) \
  { \
    if (translate) \
      { \
        return utils::Utf8String::from_utf8_encoded_string ( \
          fmt::format ("{:t}", val)); \
      } \
    return utils::Utf8String::from_utf8_encoded_string ( \
      fmt::format ("{}", val)); \
  } \
\
  inline enum_type enum_name##_from_string ( \
    const utils::Utf8String &str, bool translate = false) \
  { \
    for (size_t i = 0; i < VA_ARGS_SIZE (__VA_ARGS__); ++i) \
      { \
        if ( \
          str == enum_name##_to_string (static_cast<enum_type> (i), translate)) \
          { \
            return static_cast<enum_type> (i); \
          } \
      } \
    z_error ("Enum value from '{}' not found", str); \
    return static_cast<enum_type> (0); \
  }

/**
 * @brief Used when fmt::format can't be used (when the format string is not a
 * literal)
 */
template <typename... Args>
std::string
format_str (std::string_view format, Args &&... args)
{
  return fmt::vformat (format, fmt::make_format_args (args...));
}

template <typename... Args>
QString
format_qstr (const QString &format, Args &&... args)
{
  return utils::Utf8String::from_utf8_encoded_string (
           format_str (utils::Utf8String::from_qstring (format).str (), args...))
    .to_qstring ();
}

// Formatter for std::filesystem::path
template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const std::filesystem::path &p, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      utils::Utf8String::from_path (p).view (), ctx);
  }
};

// Formatter for juce::String
template <>
struct fmt::formatter<juce::String> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const juce::String &s, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      utils::Utf8String::from_juce_string (s).view (), ctx);
  }
};

// Formatter for QString
template <> struct fmt::formatter<QString> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const QString &s, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      utils::Utf8String::from_qstring (s).view (), ctx);
  }
};

// Generic formatter for smart pointers (std::shared_ptr and std::unique_ptr)
template <SmartPtr Ptr>
struct fmt::formatter<Ptr> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const Ptr &ptr, FormatContext &ctx) const
  {
    if (ptr)
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *ptr), ctx);
      }

    return fmt::formatter<std::string_view>::format ("(null)", ctx);
  }
};

// Formatter for std::optional
template <typename T>
struct fmt::formatter<std::optional<T>> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const std::optional<T> &opt, FormatContext &ctx) const
  {
    if (opt.has_value ())
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *opt), ctx);
      }
    return fmt::formatter<std::string_view>::format ("(nullopt)", ctx);
  }
};

// Universal formatter for all types described by BOOST_DESCRIBE_STRUCT or
// BOOST_DESCRIBE_CLASS.
template <class T>
struct fmt::formatter<
  T,
  char,
  std::enable_if_t<
    boost::describe::has_describe_bases<T>::value
    && boost::describe::has_describe_members<T>::value && !std::is_union_v<T>>>
{
  constexpr auto parse (format_parse_context &ctx)
  {
    const auto * it = ctx.begin ();
    const auto * end = ctx.end ();

    if (it != end && *it != '}')
      {
        throw std::format_error ("invalid format");
      }

    return it;
  }

  auto format (T const &t, format_context &ctx) const
  {
    using namespace boost::describe;

    using Bd = describe_bases<T, mod_any_access>;
    using Md = describe_members<T, mod_any_access>;

    auto out = ctx.out ();

    *out++ = '{';

    bool first = true;

    boost::mp11::mp_for_each<Bd> ([&] (auto D) {
      if (!first)
        {
          *out++ = ',';
        }

      first = false;

      out = fmt::format_to (out, " {}", (typename decltype (D)::type const &) t);
    });

    boost::mp11::mp_for_each<Md> ([&] (auto D) {
      if (!first)
        {
          *out++ = ',';
        }

      first = false;

      out = fmt::format_to (out, " .{}={}", D.name, t.*D.pointer);
    });

    if (!first)
      {
        *out++ = ' ';
      }

    *out++ = '}';

    return out;
  }
};

/**
 * @}
 */
