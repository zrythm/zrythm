// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <string_view>

#include <nlohmann/json.hpp>

namespace zrythm::utils
{

/**
 * @brief Key constants for version JSON serialization.
 */
namespace version_keys
{
using namespace std::literals;
inline constexpr auto kMajor = "major"sv;
inline constexpr auto kMinor = "minor"sv;
inline constexpr auto kPatch = "patch"sv;
}

/**
 * @brief Represents a semantic version with major, minor, and optional patch.
 */
struct Version
{
  int                major;
  int                minor;
  std::optional<int> patch;

  [[nodiscard]] constexpr bool operator== (const Version &other) const
  {
    return major == other.major && minor == other.minor
           && patch.value_or (0) == other.patch.value_or (0);
  }

  [[nodiscard]] constexpr bool operator!= (const Version &other) const
  {
    return !(*this == other);
  }

  [[nodiscard]] constexpr bool operator< (const Version &other) const
  {
    if (major != other.major)
      return major < other.major;
    if (minor != other.minor)
      return minor < other.minor;
    auto this_patch = patch.value_or (0);
    auto other_patch = other.patch.value_or (0);
    return this_patch < other_patch;
  }

  [[nodiscard]] constexpr bool operator> (const Version &other) const
  {
    return other < *this;
  }

  [[nodiscard]] constexpr bool operator<= (const Version &other) const
  {
    return !(other < *this);
  }

  [[nodiscard]] constexpr bool operator>= (const Version &other) const
  {
    return !(*this < other);
  }
};

inline void
to_json (nlohmann::json &j, const Version &v)
{
  using namespace version_keys;
  j = nlohmann::json::object ();
  j[kMajor] = v.major;
  j[kMinor] = v.minor;
  if (v.patch.has_value ())
    {
      j[kPatch] = *v.patch;
    }
}

inline void
from_json (const nlohmann::json &j, Version &v)
{
  using namespace version_keys;
  v.major = j.at (kMajor).get<int> ();
  v.minor = j.at (kMinor).get<int> ();
  if (j.contains (kPatch))
    {
      v.patch = j[kPatch].get<int> ();
    }
  else
    {
      v.patch = std::nullopt;
    }
}

} // namespace zrythm::utils
