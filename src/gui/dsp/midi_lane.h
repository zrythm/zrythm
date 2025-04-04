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
  MidiLane (const DeserializationDependencyHolder &dh)
      : MidiLane (
          &dh.get<std::reference_wrapper<LanedTrackImpl<MidiLane>>> ().get ())
  {
  }
  MidiLane (LanedTrackImpl<MidiLane> * track);

  void init_after_cloning (const MidiLane &other, ObjectCloneType clone_type) override;

  DECLARE_DEFINE_FIELDS_METHOD ();
};
