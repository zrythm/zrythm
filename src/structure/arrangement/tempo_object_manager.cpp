// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/tempo_object_manager.h"

namespace zrythm::structure::arrangement
{
TempoObjectManager::TempoObjectManager (
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : utils::UuidIdentifiableObject (parent),
      ArrangerObjectOwner<TempoObject> (registry, *this),
      ArrangerObjectOwner<TimeSignatureObject> (registry, *this)
{
}

void
TempoObjectManager::sync_to_tempo_map (dsp::TempoMapWrapper &wrapper)
{
  wrapper.clearTimeSignatureEvents ();
  wrapper.clearTempoEvents ();
  for (
    const auto * obj :
    ArrangerObjectOwner<TimeSignatureObject>::get_sorted_children_view ())
    {
      wrapper.addTimeSignatureEvent (
        static_cast<int64_t> (std::round (obj->position ()->ticks ())),
        obj->numerator (), obj->denominator ());
    }
  for (
    const auto * obj :
    ArrangerObjectOwner<TempoObject>::get_sorted_children_view ())
    {
      wrapper.addTempoEvent (
        static_cast<int64_t> (std::round (obj->position ()->ticks ())),
        obj->tempo (), obj->curve ());
    }
}

void
init_from (
  TempoObjectManager       &obj,
  const TempoObjectManager &other,
  utils::ObjectCloneType    clone_type)
{
  init_from (
    static_cast<ArrangerObjectOwner<TempoObject> &> (obj),
    static_cast<const ArrangerObjectOwner<TempoObject> &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<TimeSignatureObject> &> (obj),
    static_cast<const ArrangerObjectOwner<TimeSignatureObject> &> (other),
    clone_type);
}

void
to_json (nlohmann::json &j, const TempoObjectManager &manager)
{
  to_json (j, static_cast<const ArrangerObjectOwner<TempoObject> &> (manager));
  to_json (
    j, static_cast<const ArrangerObjectOwner<TimeSignatureObject> &> (manager));
}

void
from_json (const nlohmann::json &j, TempoObjectManager &manager)
{
  from_json (j, static_cast<ArrangerObjectOwner<TempoObject> &> (manager));
  from_json (
    j, static_cast<ArrangerObjectOwner<TimeSignatureObject> &> (manager));
}

} // namespace zrythm::structure::arrangement
