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
      if (ref.registry_ == nullptr)
        {
          throw std::runtime_error (
            "deserializing a UUID into a reference without a registry");
        }
      // acquire through a fresh reference first: if the registry rejects
      // the id, @p ref keeps its previous state instead of holding an id
      // it never acquired
      UuidReference new_ref{ new_id, *ref.registry_ };
      ref = std::move (new_ref);
    }
}

} // namespace zrythm::utils
