// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class MasterTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MasterTrack (FinalTrackDependencies dependencies);

  friend void init_from (
    MasterTrack           &obj,
    const MasterTrack     &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const MasterTrack &project)
  {
    to_json (j, static_cast<const Track &> (project));
  }
  friend void from_json (const nlohmann::json &j, MasterTrack &project)
  {
    from_json (j, static_cast<Track &> (project));
  }
};
}
