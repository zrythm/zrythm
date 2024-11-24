// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "track_lane.h"

class MidiLane final
    : public QObject,
      public TrackLaneImpl<MidiRegion>,
      public ICloneable<MidiLane>,
      public zrythm::utils::serialization::ISerializable<MidiLane>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_LANE_QML_PROPERTIES (MidiLane)

public:
  using RegionT = MidiRegion;

public:
  MidiLane ();

  /**
   * @see TrackLaneImpl::TrackLaneImpl
   */
  MidiLane (LanedTrackImpl<MidiLane> * track, int pos);

  void init_after_cloning (const MidiLane &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();
};