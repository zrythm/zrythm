// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "utils/variant_helpers.h"

namespace zrythm::structure::arrangement
{

/**
 * @brief Variant of raw pointers to every arranger-object owner base.
 *
 * Each alternative is ArrangerObjectOwner<ObjectT> for one object type in
 * ArrangerObjectVariant; the concrete object behind the pointer is the
 * owner (a track, lane, clip, AutomationTrack, TempoObjectManager, ...).
 *
 * Lives outside arranger_object_owner.h because instantiating the variant
 * requires complete child object types, which that header cannot include
 * (every concrete object header includes it).
 */
using ArrangerObjectOwnerPtrVariant = utils::to_pointer_variant<
  utils::wrap_variant_t<ArrangerObjectVariant, ArrangerObjectOwner>>;

} // namespace zrythm::structure::arrangement
