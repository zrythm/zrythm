// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/chord_object.h"
#include "utils/qt.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
ChordObject::ChordObject (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  QObject *                   parent)
    : ArrangerObject (
        Type::ChordObject,
        tempo_map_wrapper,
        ArrangerObjectFeatures::Mute,
        parent),
      chord_descriptor_ (utils::make_qobject_unique<dsp::ChordDescriptor> ())
{
  QObject::connect (
    chord_descriptor_.get (), &dsp::ChordDescriptor::changed, this,
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

  const auto * src = other.chordDescriptor ();
  auto *       dest = obj.chordDescriptor ();
  dest->setRootNote (src->rootNote ());
  dest->setHasBass (src->hasBass ());
  dest->setBassNote (src->bassNote ());
  dest->setChordType (src->chordType ());
  dest->setChordAccent (src->chordAccent ());
  dest->setInversion (src->inversion ());
}

void
to_json (nlohmann::json &j, const ChordObject &co)
{
  to_json (j, static_cast<const ArrangerObject &> (co));
  to_json (j[ChordObject::kChordDescriptorKey], *co.chordDescriptor ());
}

void
from_json (const nlohmann::json &j, ChordObject &co)
{
  from_json (j, static_cast<ArrangerObject &> (co));
  from_json (j.at (ChordObject::kChordDescriptorKey), *co.chordDescriptor ());
}
}
