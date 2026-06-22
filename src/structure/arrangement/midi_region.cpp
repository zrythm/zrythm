// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/midi_region.h"

namespace zrythm::structure::arrangement
{
MidiRegion::MidiRegion (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  utils::IObjectRegistry     &object_registry,
  QObject *                   parent)
    : ArrangerObject (
        Type::MidiRegion,
        tempo_map_wrapper,
        ArrangerObjectFeatures::Region,
        parent),
      ArrangerObjectOwner<MidiNote> (object_registry, *this),
      ArrangerObjectOwner<MidiControlEvent> (object_registry, *this)
{
  QObject::connect (
    midiNotes (), &ArrangerObjectListModel::contentChanged, this,
    &MidiRegion::contentChanged);
  QObject::connect (
    midiControlEvents (), &ArrangerObjectListModel::contentChanged, this,
    &MidiRegion::contentChanged);
}

void
init_from (
  MidiRegion            &obj,
  const MidiRegion      &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<MidiNote> &> (obj),
    static_cast<const ArrangerObjectOwner<MidiNote> &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<MidiControlEvent> &> (obj),
    static_cast<const ArrangerObjectOwner<MidiControlEvent> &> (other),
    clone_type);
}
}
