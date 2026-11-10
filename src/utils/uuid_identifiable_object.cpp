// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/serialization.h"
#include "utils/uuid_identifiable_object.h"

#include <nlohmann/json.hpp>

namespace zrythm::utils
{

void
to_json (nlohmann::json &j, const UuidIdentifiableObject &obj)
{
  j["id"] = obj.raw_uuid ();
}

void
from_json (const nlohmann::json &j, UuidIdentifiableObject &obj)
{
  j.at ("id").get_to (obj.uuid_);
}

} // namespace zrythm::utils
