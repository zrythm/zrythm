// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "track_lane.h"

namespace zrythm::structure::tracks
{
class AudioLane final
    : public QObject,
      public TrackLaneImpl<arrangement::AudioRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_LANE_QML_PROPERTIES (AudioLane, arrangement::AudioRegion)

public:
  using AudioRegion = arrangement::AudioRegion;
  using RegionT = AudioRegion;

public:
  /**
   * @see TrackLaneImpl::TrackLaneImpl
   */
  AudioLane (
    structure::arrangement::ArrangerObjectRegistry &registry,
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
    LanedTrackImpl<AudioLane> *                     track);

  friend void init_from (
    AudioLane             &obj,
    const AudioLane       &other,
    utils::ObjectCloneType clone_type);

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
}
