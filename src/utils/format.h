// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <filesystem>
#include <source_location>

#include "utils/qt.h"
#include "utils/string.h"
#include "utils/traits.h"

#include <QObject>
#include <QString>
#include <QUuid>

#include <juce_wrapper.h>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <boost/describe.hpp>
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
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
  inline utils::Utf8String \
  enum_name##_to_string (enum_type val, bool translate = false) \
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

// Formatter for juce::File
template <> struct fmt::formatter<juce::File> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const juce::File &s, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      utils::Utf8String::from_juce_string (s.getFullPathName ()).view (), ctx);
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

// Formatter for std::atomic
template <typename T>
struct fmt::formatter<std::atomic<T>> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const std::atomic<T> &opt, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      fmt::format ("{}", opt.load ()), ctx);
  }
};

// Formatter for QPointer
template <typename T>
struct fmt::formatter<QPointer<T>> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const QPointer<T> &opt, FormatContext &ctx) const
  {
    if (!opt.isNull ())
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *opt), ctx);
      }
    return fmt::formatter<std::string_view>::format ("(null)", ctx);
  }
};

// Formatter for utils::QObjectUniquePtr
template <typename T>
struct fmt::formatter<utils::QObjectUniquePtr<T>>
    : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const utils::QObjectUniquePtr<T> &opt, FormatContext &ctx) const
  {
    if (!opt)
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *opt), ctx);
      }
    return fmt::formatter<std::string_view>::format ("(null)", ctx);
  }
};

// Formatter for QUuid
template <> struct fmt::formatter<QUuid> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const QUuid &uuid, FormatContext &ctx) const
  {
    return fmt::formatter<QString>{}.format (
      uuid.toString (QUuid::WithoutBraces), ctx);
  }
};

// Formatter for source_location
template <>
struct fmt::formatter<std::source_location> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const std::source_location &loc, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      fmt::format (
        "{}:{}:{}:{}", loc.file_name (), loc.function_name (), loc.line (),
        loc.column ()),
      ctx);
  }
};

// Formatter for std::variant
template <typename... Ts>
struct fmt::formatter<std::variant<Ts...>> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const std::variant<Ts...> &variant, FormatContext &ctx) const
  {
    return std::visit (
      [&] (auto &&val) {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", val), ctx);
      },
      variant);
  }
};

// Formatter for enum classes
template <EnumType T>
struct fmt::formatter<T> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const T &val, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>::format (
      fmt::format ("{}", ENUM_NAME (val)), ctx);
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
