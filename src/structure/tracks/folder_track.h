// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{

/**
 * @brief A track that can contain other tracks.
 */
class FolderTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  FolderTrack (FinalTrackDependencies dependencies);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

private:
  friend void to_json (nlohmann::json &j, const FolderTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, FolderTrack &track);

  friend void init_from (
    FolderTrack           &obj,
    const FolderTrack     &other,
    utils::ObjectCloneType clone_type);
};

} // namespace zrythm::structure::tracks
