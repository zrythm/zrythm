// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "utils/version.h"

#include <nlohmann/json.hpp>
#include <scn/scan.h>

namespace zrythm::utils
{
void
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

void
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

Utf8String
get_app_version_string (bool with_v)
{
  constexpr const char * ver = PACKAGE_VERSION;

  return utils::Utf8String::from_utf8_encoded_string (
    [ver, with_v] () -> std::string {
      if (with_v)
        {
          if (ver[0] == 'v')
            return ver;

          return fmt::format ("v{}", ver);
        }
      else
        {
          if (ver[0] == 'v')
            return &ver[1];

          return ver;
        }
    }());
}

Version
get_app_version ()
{
  // Parse PACKAGE_VERSION (e.g., "2.0.0" or "v2.0.0")
  constexpr std::string_view ver = PACKAGE_VERSION;
  constexpr std::string_view clean_ver = (ver[0] == 'v') ? ver.substr (1) : ver;

  int  major = 0;
  int  minor = 0;
  int  patch = 0;
  auto result = scn::scan<int, int, int> (clean_ver, "{}.{}.{}");
  if (result)
    {
      std::tie (major, minor, patch) = result->values ();
    }

  return utils::Version{
    .major = major,
    .minor = minor,
    .patch = patch > 0 ? std::optional<int>{ patch } : std::nullopt
  };
}
} // namespace zrythm::utils
