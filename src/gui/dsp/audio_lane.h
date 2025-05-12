// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "track_lane.h"

class AudioLane final
    : public QObject,
      public TrackLaneImpl<AudioRegion>,
      public ICloneable<AudioLane>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_LANE_QML_PROPERTIES (AudioLane, AudioRegion)

public:
  using RegionT = AudioRegion;

public:
  /**
   * @see TrackLaneImpl::TrackLaneImpl
   */
  AudioLane (LanedTrackImpl<AudioLane> * track);

  void init_after_cloning (const AudioLane &other, ObjectCloneType clone_type)
    override;

private:
  friend void to_json (nlohmann::json &j, const AudioLane &lane)
  {
    to_json (j, static_cast<const TrackLaneImpl &> (lane));
  }
  friend void from_json (const nlohmann::json &j, AudioLane &lane)
  {
    from_json (j, static_cast<TrackLaneImpl &> (lane));
  }
};
