// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/muteable_object.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
void
to_json (nlohmann::json &j, const ArrangerObjectMuteFunctionality &object)
{
  j[ArrangerObjectMuteFunctionality::kMutedKey] = object.muted_;
}
void
from_json (const nlohmann::json &j, ArrangerObjectMuteFunctionality &object)
{
  j.at (ArrangerObjectMuteFunctionality::kMutedKey).get_to (object.muted_);
  Q_EMIT object.mutedChanged (object.muted_);
}

} // namespace zrythm::structure::arrangement
