// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <vector>

#include "commands/command.h"
#include "structure/arrangement/arranger_object.h"

namespace zrythm::commands
{

class MoveArrangerObjectsCommand : public Command
{
public:
  enum class VerticalChangeType : std::uint8_t
  {
    Pitch,
    Velocity,
    AutomationValue,
  };

  /**
   * @param engine_requirement Engine cooperation this move needs: pass
   * PauseProcessing when the selection contains tempo-map-affecting
   * objects, None otherwise.
   */
  MoveArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    units::precise_tick_t                                            tick_delta,
    EngineRequirement  engine_requirement,
    double             vertical_delta = 0.0,
    VerticalChangeType vertical_change_type = VerticalChangeType::Pitch);

  bool merge_with (const Command &other) override;

  void undo () override;
  void redo () override;

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects_;
  std::vector<units::precise_tick_t> original_positions_;
  std::vector<double>                original_vertical_positions_;
  units::precise_tick_t              tick_delta_;
  double                             vertical_delta_{};
  VerticalChangeType                 vertical_change_type_{};
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
