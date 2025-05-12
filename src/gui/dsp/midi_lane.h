// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "track_lane.h"

class MidiLane final
    : public QObject,
      public TrackLaneImpl<MidiRegion>,
      public ICloneable<MidiLane>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_LANE_QML_PROPERTIES (MidiLane, MidiRegion)

public:
  using RegionT = MidiRegion;

public:
  MidiLane (LanedTrackImpl<MidiLane> * track);

  void init_after_cloning (const MidiLane &other, ObjectCloneType clone_type)
    override;

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
