// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/midi_region.h"

namespace zrythm::structure::arrangement
{
MidiRegion::MidiRegion (
  const dsp::TempoMap          &tempo_map,
  ArrangerObjectRegistry       &object_registry,
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  QObject *                     parent)
    : ArrangerObject (
        Type::MidiRegion,
        tempo_map,
        ArrangerObjectFeatures::Region,
        parent),
      ArrangerObjectOwner (object_registry, file_audio_source_registry, *this)
{
  QObject::connect (
    midiNotes (), &ArrangerObjectListModel::contentChanged, this,
    &MidiRegion::contentChanged);
}

int
MidiRegion::minVisiblePitch () const
{
  if (get_children_vector ().empty ())
    return 0;

  auto pitches = get_children_view () | std::views::transform (&MidiNote::pitch);
  return std::ranges::min (pitches);
}

int
MidiRegion::maxVisiblePitch () const
{
  if (get_children_vector ().empty ())
    return 0;

  auto pitches = get_children_view () | std::views::transform (&MidiNote::pitch);
  return std::ranges::max (pitches);
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
}
}
