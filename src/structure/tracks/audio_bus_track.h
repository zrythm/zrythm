// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief An audio bus track that can be processed and has channels.
 */
class AudioBusTrack : public Track
{
public:
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  AudioBusTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    AudioBusTrack         &obj,
    const AudioBusTrack   &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const AudioBusTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, AudioBusTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }
};
}
