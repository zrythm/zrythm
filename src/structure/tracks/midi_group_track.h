// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class MidiGroupTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiGroupTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    MidiGroupTrack        &obj,
    const MidiGroupTrack  &other,
    utils::ObjectCloneType clone_type);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

private:
  friend void to_json (nlohmann::json &j, const MidiGroupTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, MidiGroupTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }
};
}
