// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <type_traits>

namespace zrythm::utils::serialization
{

/**
 * @brief Opt-out marker for enums with their own to_json/from_json JSON
 * serialization, using names other than their enumerator names.
 *
 * Specialize this to std::true_type next to the enum definition; the
 * declaration must live in the enum's header so that every translation unit
 * serializing the enum sees the same value. The central enum serializer in
 * utils/serialization.h leaves such enums alone.
 */
template <typename T> struct use_custom_enum_json : std::false_type
{
};

} // namespace zrythm::utils::serialization

/**
 * Marks @p _enum as having its own to_json/from_json JSON serialization.
 * Invoke at global scope, after the namespace of @p _enum closes.
 */
#define ENUM_CUSTOM_JSON(_enum) \
  template <> \
  struct zrythm::utils::serialization::use_custom_enum_json<_enum> \
      : std::true_type \
  { \
  }
