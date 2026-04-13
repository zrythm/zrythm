// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#elifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4458) // declaration hides class member
#endif

#include <magic_enum.hpp>
#include <magic_enum_format.hpp>

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#elifdef _MSC_VER
#  pragma warning(pop)
#endif

#include "utils/utf8_string.h"

#include <QObject>

#include <fmt/format.h>

using namespace magic_enum::bitwise_operators;

#define ENUM_INT_TO_VALUE_CONST(_enum, _int) \
  (magic_enum::enum_value<_enum, _int> ())
#define ENUM_INT_TO_VALUE(_enum, _int) (magic_enum::enum_value<_enum> (_int))
#define ENUM_VALUE_TO_INT(_val) (magic_enum::enum_integer (_val))

#define ENUM_ENABLE_BITSET(_enum) \
  template <> struct magic_enum::customize::enum_range<_enum> \
  { \
    static constexpr bool is_flags = true; \
  }
#define ENUM_BITSET(_enum, _val) (magic_enum::containers::bitset<_enum> (_val))
#define ENUM_BITSET_TEST(_val, _other_val) \
  /* (ENUM_BITSET (_enum, _val).test (_other_val)) */ \
  (static_cast<std::underlying_type_t<decltype (_val)>> (_val) \
   & static_cast<std::underlying_type_t<decltype (_val)>> (_other_val))

/** @important ENUM_ENABLE_BITSET must be called on the enum that this is used
 * on. */
#define ENUM_BITSET_TO_STRING(_enum, _val) \
  (ENUM_BITSET (_enum, _val).to_string ().data ())

#define ENUM_COUNT(_enum) (magic_enum::enum_count<_enum> ())
#define ENUM_NAME(_val) (magic_enum::enum_name (_val).data ())
#define ENUM_NAME_FROM_INT(_enum, _int) \
  ENUM_NAME (ENUM_INT_TO_VALUE (_enum, _int))

#define VA_ARGS_SIZE(...) \
  std::tuple_size<decltype (std::make_tuple (__VA_ARGS__))>::value

#define DEFINE_ENUM_FORMATTER(enum_type, enum_name, ...) \
  namespace fmt \
  { \
  template <> struct formatter<enum_type> \
  { \
    bool                                            translate_ = false; \
    template <typename ParseContext> constexpr auto parse (ParseContext &ctx) \
    { \
      auto it = ctx.begin (); \
      translate_ = (it != ctx.end () && *it == 't'); \
      return it + translate_; \
    } \
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
          ? zrythm::utils::Utf8String::from_qstring (QObject::tr (str)).c_str () \
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
  inline zrythm::utils::Utf8String \
  enum_name##_to_string (enum_type val, bool translate = false) \
  { \
    if (translate) \
      { \
        return zrythm::utils::Utf8String::from_utf8_encoded_string ( \
          fmt::format ("{:t}", val)); \
      } \
    return zrythm::utils::Utf8String::from_utf8_encoded_string ( \
      fmt::format ("{}", val)); \
  } \
\
  inline enum_type enum_name##_from_string ( \
    const zrythm::utils::Utf8String &str, bool translate = false) \
  { \
    for (size_t i = 0; i < VA_ARGS_SIZE (__VA_ARGS__); ++i) \
      { \
        if ( \
          str == enum_name##_to_string (static_cast<enum_type> (i), translate)) \
          { \
            return static_cast<enum_type> (i); \
          } \
      } \
    throw std::runtime_error ( \
      fmt::format ("Enum value from '{}' not found", str)); \
    return static_cast<enum_type> (0); \
  }
