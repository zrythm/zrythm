// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/version.h"

#include <nlohmann/json.hpp>

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
} // namespace zrythm::utils
