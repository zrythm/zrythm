// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class InstrumentTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  InstrumentTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    InstrumentTrack       &obj,
    const InstrumentTrack &other,
    utils::ObjectCloneType clone_type);

private:
  friend void
  to_json (nlohmann::json &j, const InstrumentTrack &instrument_track)
  {
    to_json (j, static_cast<const Track &> (instrument_track));
  }
  friend void
  from_json (const nlohmann::json &j, InstrumentTrack &instrument_track)
  {
    from_json (j, static_cast<Track &> (instrument_track));
  }
};
}
