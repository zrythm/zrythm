// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/marker.h"

namespace zrythm::structure::arrangement
{
Marker::Marker (const dsp::TempoMap &tempo_map, MarkerType type, QObject * parent)
    : QObject (parent),
      ArrangerObject (ArrangerObject::Type::Marker, tempo_map, *this),
      marker_type_ (type),
      name_ (utils::make_qobject_unique<ArrangerObjectName> (this))
{
}

void
init_from (Marker &obj, const Marker &other, utils::ObjectCloneType clone_type)

{
  obj.marker_type_ = other.marker_type_;
  init_from (*obj.name_, *other.name_, clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}
}
