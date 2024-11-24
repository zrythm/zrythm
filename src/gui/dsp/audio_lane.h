// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "track_lane.h"

class AudioLane final
    : public QObject,
      public TrackLaneImpl<AudioRegion>,
      public ICloneable<AudioLane>,
      public zrythm::utils::serialization::ISerializable<AudioLane>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_LANE_QML_PROPERTIES (AudioLane)

public:
  using RegionT = MidiRegion;

public:
  AudioLane ();
  /**
   * @see TrackLaneImpl::TrackLaneImpl
   */
  AudioLane (LanedTrackImpl<AudioLane> * track, int pos);

  void init_after_cloning (const AudioLane &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();
};