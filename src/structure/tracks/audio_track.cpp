// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>

#include "structure/tracks/audio_track.h"

namespace zrythm::structure::tracks
{

AudioTrack::AudioTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Audio,
        PortType::Audio,
        PortType::Audio,
        TrackFeatures::Automation | TrackFeatures::Lanes | TrackFeatures::Recording,
        dependencies.to_base_dependencies ())
{
  color_ = Color (QColor ("#2BD700"));
  /* signal-audio also works */
  icon_name_ = u8"view-media-visualization";

  processor_ = make_track_processor ();
}

void
init_from (
  AudioTrack            &obj,
  const AudioTrack      &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
