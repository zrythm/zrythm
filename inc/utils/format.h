// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_FORMAT_H__
#define __UTILS_FORMAT_H__

#include "zrythm-config.h"

#include <QString>
#include <filesystem>

#include "utils/traits.h"

#include "ext/juce/juce.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <magic_enum.hpp>

/**
 * @addtogroup utils
 *
 * @{
 */

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
 * 1. Using a lambda:
 * DEFINE_OBJECT_FORMATTER(MyClass, [](const MyClass& obj) {
 *     return std::to_string(obj.getValue());
 * })
 *
 * 2. Using a named function:
 * std::string formatMyClass(const MyClass& obj) {
 *     return std::to_string(obj.getValue());
 * }
 * DEFINE_OBJECT_FORMATTER(MyClass, formatMyClass)
 *
 * @param obj_type The object type to define the formatter for.
 * @param formatter_func The function to use for formatting the object.
 */
#define DEFINE_OBJECT_FORMATTER(obj_type, formatter_func) \
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
  inline std::string obj_type##__to_string (const obj_type &obj) \
  { \
    return fmt::format ("{}", obj); \
  } \
\
  inline void obj_type##__print (const obj_type &obj) \
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
        ctx.out (), FMT_STRING ("{}"), translate_ ? _ (str) : str); \
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
  inline std::string enum_name##_to_string ( \
    enum_type val, bool translate = false) \
  { \
    if (translate) \
      { \
        return fmt::format ("{:t}", val); \
      } \
    return fmt::format ("{}", val); \
  } \
\
  inline enum_type enum_name##_from_string ( \
    std::string_view str, bool translate = false) \
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

// Formatter for std::filesystem::path
template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const std::filesystem::path &p, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (p.string (), ctx);
  }
};

// Formatter for juce::String
template <>
struct fmt::formatter<juce::String> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const juce::String &s, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (s.toStdString (), ctx);
  }
};

// Formatter for QString
template <> struct fmt::formatter<QString> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const QString &s, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (s.toStdString (), ctx);
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

/**
 * @}
 */

#endif