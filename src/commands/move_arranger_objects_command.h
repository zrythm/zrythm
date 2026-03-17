// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <vector>

#include "structure/arrangement/arranger_object.h"

#include <QUndoCommand>

namespace zrythm::commands
{

class MoveArrangerObjectsCommand : public QUndoCommand
{
public:
  enum class VerticalChangeType : std::uint8_t
  {
    Pitch,
    Velocity,
    AutomationValue,
  };

  MoveArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    units::precise_tick_t                                            tick_delta,
    double             vertical_delta = 0.0,
    VerticalChangeType vertical_change_type = VerticalChangeType::Pitch);

  int id () const override { return 894553188; }

  bool mergeWith (const QUndoCommand * other) override;

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

class MoveTempoMapAffectingArrangerObjectsCommand
    : public MoveArrangerObjectsCommand
{
public:
  static constexpr int CommandId = 1762956469;
  using MoveArrangerObjectsCommand::MoveArrangerObjectsCommand;

  int id () const override { return CommandId; }
};

} // namespace zrythm::commands
