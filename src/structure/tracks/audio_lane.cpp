// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "./audio_lane.h"

namespace zrythm::structure::tracks
{
AudioLane::AudioLane (
  structure::arrangement::ArrangerObjectRegistry &registry,
  dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
  LanedTrackImpl<AudioLane> *                     track)
    : TrackLaneImpl<AudioRegion> (registry, file_audio_source_registry, track, *this)
{
}

void
init_from (
  AudioLane             &obj,
  const AudioLane       &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<AudioLane::ArrangerObjectOwner &> (obj),
    static_cast<const AudioLane::ArrangerObjectOwner &> (other), clone_type);
  init_from (
    static_cast<AudioLane::TrackLaneImpl &> (obj),
    static_cast<const AudioLane::TrackLaneImpl &> (other), clone_type);
}
}
