// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <ranges>
#include <vector>

#include "structure/arrangement/arranger_object_all.h"

#include <QUndoCommand>

namespace zrythm::commands
{
Q_NAMESPACE

/// Enumeration of supported resize types
enum class ResizeType : int
{
  Bounds = 0,
  LoopPoints = 1,
  Fades = 2
};
Q_ENUM_NS (ResizeType)

/// Enumeration of resize directions
enum class ResizeDirection : int
{
  FromStart = 0,
  FromEnd = 1
};
Q_ENUM_NS (ResizeDirection)

class ResizeArrangerObjectsCommand : public QUndoCommand
{
  static constexpr int ID = 1762503480;

public:
  /// Structure to hold original state of an arranger object for undo
  struct OriginalState
  {
    units::precise_tick_t position = units::ticks (0);
    units::precise_tick_t length = units::ticks (0);
    units::precise_tick_t clipStart = units::ticks (0);
    units::precise_tick_t loopStart = units::ticks (0);
    units::precise_tick_t loopEnd = units::ticks (0);
    bool                  boundsTracked{};
    units::precise_tick_t fadeInOffset = units::ticks (0);
    units::precise_tick_t fadeOutOffset = units::ticks (0);
    units::precise_tick_t firstChildTicks = units::ticks (0);
  };

  ResizeArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    ResizeType                                                       type,
    ResizeDirection                                                  direction,
    double                                                           delta);

  int id () const override { return ID; }

  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;

    // Only merge if other command was made in quick succession of this
    // command's redo() and operates on the same objects
    const auto * other_cmd =
      dynamic_cast<const ResizeArrangerObjectsCommand *> (other);
    const auto cur_time = std::chrono::steady_clock::now ();
    const auto duration = cur_time - last_redo_timestamp_;
    if (
      std::chrono::duration_cast<std::chrono::milliseconds> (duration).count ()
      > 1'000)
      {
        return false;
      }

    // Check if we're operating on the same set of objects, type, and direction
    if (type_ != other_cmd->type_ || direction_ != other_cmd->direction_)
      return false;

    if (objects_.size () != other_cmd->objects_.size ())
      return false;

    if (
      !std::ranges::equal (
        objects_, other_cmd->objects_, {},
        &structure::arrangement::ArrangerObjectUuidReference::id,
        &structure::arrangement::ArrangerObjectUuidReference::id))
      {
        return false;
      }

    last_redo_timestamp_ = cur_time;
    delta_ += other_cmd->delta_;
    return true;
  }

  void undo () override;
  void redo () override;

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects_;
  ResizeType                                                       type_;
  ResizeDirection                                                  direction_;
  double                                                           delta_;

  // Original state storage for undo
  std::vector<OriginalState> original_states_;

  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
