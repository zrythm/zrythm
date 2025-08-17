// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief A track that processes MIDI data.
 */
class MidiBusTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiBusTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    MidiBusTrack          &obj,
    const MidiBusTrack    &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const MidiBusTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, MidiBusTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }
};
}
