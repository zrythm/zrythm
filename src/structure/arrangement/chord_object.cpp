// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/chord_object.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
ChordObject::ChordObject (const dsp::TempoMap &tempo_map, QObject * parent)
    : ArrangerObject (Type::ChordObject, tempo_map, ArrangerObjectFeatures::Mute, parent)
{
  QObject::connect (
    this, &ChordObject::chordDescriptorIndexChanged, this,
    &ArrangerObject::propertiesChanged);
}

void
init_from (
  ChordObject           &obj,
  const ChordObject     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  obj.chord_index_ = other.chord_index_;
}

void
ChordObject::setChordDescriptorIndex (int descr)
{
  if (descr == chord_index_)
    return;
  chord_index_ = descr;
  Q_EMIT chordDescriptorIndexChanged (descr);
}

void
to_json (nlohmann::json &j, const ChordObject &co)
{
  to_json (j, static_cast<const ArrangerObject &> (co));
  j[ChordObject::kChordIndexKey] = co.chord_index_;
}

void
from_json (const nlohmann::json &j, ChordObject &co)
{
  from_json (j, static_cast<ArrangerObject &> (co));
  j.at (ChordObject::kChordIndexKey).get_to (co.chord_index_);
}
}
