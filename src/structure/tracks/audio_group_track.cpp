// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/audio_group_track.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::tracks
{
AudioGroupTrack::AudioGroupTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::AudioGroup,
        PortType::Audio,
        PortType::Audio,
        TrackFeatures::Automation,
        dependencies.to_base_dependencies ()),
      foldable_track_mixin_ (
        utils::make_qobject_unique<
          FoldableTrackMixin> (dependencies.track_registry_, this))
{
  /* GTK color picker color */
  color_ = Color (QColor ("#26A269"));
  icon_name_ = u8"effect";
  processor_ = make_track_processor ();
}

void
init_from (
  AudioGroupTrack       &obj,
  const AudioGroupTrack &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
