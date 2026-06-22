// SPDX-FileCopyrightText: © 2018-2020, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief Track containing AudioRegion's.
 */
class AudioTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using AudioRegion = arrangement::AudioRegion;

  AudioTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    AudioTrack            &obj,
    const AudioTrack      &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const AudioTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, AudioTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }

  bool initialize ();
};
}
