// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object.h"

namespace zrythm::structure::arrangement
{

ArrangerObject::ArrangerObject (
  Type                 type,
  const dsp::TempoMap &tempo_map,
  QObject *            parent) noexcept
    : QObject (parent), type_ (type), position_ (tempo_map),
      position_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (position_, this))
{
}

void
init_from (
  ArrangerObject        &obj,
  const ArrangerObject  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject::UuidIdentifiableObject &> (obj),
    static_cast<const ArrangerObject::UuidIdentifiableObject &> (other),
    clone_type);
  obj.position_.set_ticks (other.position_.get_ticks ());
}

void
to_json (nlohmann::json &j, const ArrangerObject &arranger_object)
{
  to_json (
    j,
    static_cast<const ArrangerObject::UuidIdentifiableObject &> (
      arranger_object));
  j[ArrangerObject::kPositionKey] = arranger_object.position_;
}

void
from_json (const nlohmann::json &j, ArrangerObject &arranger_object)
{
  from_json (
    j, static_cast<ArrangerObject::UuidIdentifiableObject &> (arranger_object));
  j.at (ArrangerObject::kPositionKey).get_to (arranger_object.position_);
}

} // namespace zrythm::structure:arrangement
