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

  /**
   * Inserts and connects a Modulator to the Track.
   *
   * @param replace_mode Whether to perform the operation in replace mode
   * (replace current modulator if true, not touching other modulators, or
   * push other modulators forward if false).
   */
  PluginPtrVariant insert_modulator (
    plugins::PluginSlot::SlotNo  slot,
    plugins::PluginUuidReference modulator_id,
    bool                         replace_mode,
    bool                         confirm,
    bool                         gen_automatables,
    bool                         recalc_graph,
    bool                         pub_events);

  std::optional<PluginPtrVariant>
  get_modulator (plugins::PluginSlot::SlotNo slot) const;

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
