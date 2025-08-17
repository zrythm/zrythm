// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class MidiTrack final : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    MidiTrack             &obj,
    const MidiTrack       &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const MidiTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, MidiTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }
};
}
