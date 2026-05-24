// SPDX-FileCopyrightText: © 2019-2020, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/marker.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
Marker::Marker (const dsp::TempoMap &tempo_map, MarkerType type, QObject * parent)
    : ArrangerObject (
        ArrangerObject::Type::Marker,
        tempo_map,
        ArrangerObjectFeatures::Name,
        parent),
      marker_type_ (type)
{
}

void
init_from (Marker &obj, const Marker &other, utils::ObjectCloneType clone_type)

{
  obj.marker_type_ = other.marker_type_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const Marker &m)
{
  to_json (j, static_cast<const ArrangerObject &> (m));
  j[Marker::kMarkerTypeKey] = m.marker_type_;
}
void
from_json (const nlohmann::json &j, Marker &m)
{
  from_json (j, static_cast<ArrangerObject &> (m));
  j.at (Marker::kMarkerTypeKey).get_to (m.marker_type_);
}
}
