// SPDX-FileCopyrightText: © 2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/master_track.h"

namespace zrythm::structure::tracks
{
MasterTrack::MasterTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Master,
        PortType::Audio,
        PortType::Audio,
        TrackFeatures::Automation,
        dependencies.to_base_dependencies ())
{
  /* GTK color picker color */
  color_ = Color (QColor ("#D90368"));
  icon_name_ = u8"jam-icons-crown";

  processor_ = make_track_processor ();
}

void
init_from (
  MasterTrack           &obj,
  const MasterTrack     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
}
}
