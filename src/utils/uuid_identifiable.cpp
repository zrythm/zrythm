// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/uuid_identifiable.h"

#include <nlohmann/json.hpp>

namespace zrythm::utils
{

void
to_json (nlohmann::json &j, const UuidIdentifiableBase &obj)
{
  j["id"] = obj.raw_uuid ();
}

void
from_json (const nlohmann::json &j, UuidIdentifiableBase &obj)
{
  j.at ("id").get_to (obj.uuid_);
}

} // namespace zrythm::utils
