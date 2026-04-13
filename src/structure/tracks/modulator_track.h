// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief A track that can host modulator plugins.
 */
class ModulatorTrack : public Track
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ModulatorTrack (FinalTrackDependencies dependencies);

  // ============================================================================
  // QML Interface
  // ============================================================================

  // ============================================================================

  friend void init_from (
    ModulatorTrack        &obj,
    const ModulatorTrack  &other,
    utils::ObjectCloneType clone_type);

  auto get_modulator_macro_processors () const
  {
    return std::span (modulator_macro_processors_);
  }

private:
  friend void to_json (nlohmann::json &j, const ModulatorTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
  }
  friend void from_json (const nlohmann::json &j, ModulatorTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
  }
};
}
