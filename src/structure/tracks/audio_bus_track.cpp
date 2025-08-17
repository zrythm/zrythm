// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/audio_bus_track.h"

namespace zrythm::structure::tracks
{
AudioBusTrack::AudioBusTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::AudioBus,
        PortType::Audio,
        PortType::Audio,
        TrackFeatures::Automation,
        dependencies.to_base_dependencies ())
{
  /* GTK color picker color */
  color_ = Color (QColor ("#33D17A"));
  icon_name_ = u8"effect";
  processor_ = make_track_processor ();
}

void
init_from (
  AudioBusTrack         &obj,
  const AudioBusTrack   &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
