// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_bus_track.h"

namespace zrythm::structure::tracks
{
MidiBusTrack::MidiBusTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::MidiBus,
        PortType::Midi,
        PortType::Midi,
        TrackFeatures::Automation,
        dependencies.to_base_dependencies ())
{
  color_ = Color (QColor ("#F5C211"));
  icon_name_ = u8"signal-midi";

  processor_ = make_track_processor ();
}

void
init_from (
  MidiBusTrack          &obj,
  const MidiBusTrack    &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}

}
