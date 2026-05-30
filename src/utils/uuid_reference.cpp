// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/serialization.h"
#include "utils/uuid_reference.h"

#include <nlohmann/json.hpp>

namespace zrythm::utils
{

void
to_json (nlohmann::json &j, const UuidReference &ref)
{
  if (ref.id_.has_value ())
    {
      j = ref.id_.value ();
    }
  else
    {
      j = nullptr;
    }
}

void
from_json (const nlohmann::json &j, UuidReference &ref)
{
  if (!j.is_null ())
    {
      auto new_id = j.get<QUuid> ();
      if (ref.id_.has_value () && *ref.id_ == new_id)
        return;
      ref.release_ref ();
      ref.id_ = new_id;
      ref.acquire_ref ();
    }
}

} // namespace zrythm::utils
