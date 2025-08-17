// SPDX-FileCopyrightText: © 2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_group_track.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::tracks
{
MidiGroupTrack::MidiGroupTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::MidiGroup,
        PortType::Midi,
        PortType::Midi,
        TrackFeatures::Automation,
        dependencies.to_base_dependencies ()),
      foldable_track_mixin_ (
        utils::make_qobject_unique<
          FoldableTrackMixin> (dependencies.track_registry_, this))
{
  color_ = Color (QColor ("#E66100"));
  icon_name_ = u8"signal-midi";

  processor_ = make_track_processor ();
}

void
init_from (
  MidiGroupTrack        &obj,
  const MidiGroupTrack  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
