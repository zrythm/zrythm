// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "track_lane.h"

namespace zrythm::structure::tracks
{
class MidiLane final : public QObject, public TrackLaneImpl<arrangement::MidiRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_LANE_QML_PROPERTIES (MidiLane, arrangement::MidiRegion)

public:
  using MidiRegion = arrangement::MidiRegion;
  using RegionT = MidiRegion;

public:
  MidiLane (
    structure::arrangement::ArrangerObjectRegistry &registry,
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
    LanedTrackImpl<MidiLane> *                      track);

  friend void init_from (
    MidiLane              &obj,
    const MidiLane        &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const MidiLane &lane)
  {
    to_json (j, static_cast<const TrackLaneImpl &> (lane));
  }
  friend void from_json (const nlohmann::json &j, MidiLane &lane)
  {
    from_json (j, static_cast<TrackLaneImpl &> (lane));
  }
};
}
