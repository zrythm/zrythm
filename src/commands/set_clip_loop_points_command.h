// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/command.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/clip.h"
#include "utils/typed_uuid_reference.h"

namespace zrythm::commands
{

/**
 * @brief Sets a clip's clip-start / loop-start / loop-end to absolute content
 * positions, undoably.
 *
 * The command is immutable once constructed: it captures the clip's current
 * state as @c before_ at construction and applies the given target on
 * @c redo(). There is deliberately no "update the target while on the stack"
 * pathway — live dragging is handled by mutating the clip directly during the
 * gesture and constructing (and pushing) a single command at the end (see
 * @ref actions::ClipOperator).
 *
 * The clip is held by a reference-counted @ref TypedUuidReference, so it stays
 * alive while the command is on the undo stack.
 */
class SetClipLoopPointsCommand : public Command
{
public:
  /// Snapshot of the three loop positions plus the track-bounds flag. Used to
  /// capture the "before" state for undo and the live-drag state by
  /// ClipOperator.
  struct Snapshot
  {
    dsp::ContentTick clip_start{};
    dsp::ContentTick loop_start{};
    dsp::ContentTick loop_end{};
    bool             track_bounds{ true };
  };

  /// Capture @c before_ from @p clip's current state; @p redo() will move it
  /// to the given target.
  SetClipLoopPointsCommand (
    utils::TypedUuidReference<structure::arrangement::Clip> clip,
    dsp::ContentTick                                        new_clip_start,
    dsp::ContentTick                                        new_loop_start,
    dsp::ContentTick                                        new_loop_end);

  void undo () override;
  void redo () override;

  /// Apply a loop-points target to @p clip directly: length-tracking is
  /// disabled, the three positions are written atomically (no per-set
  /// clamping via @ref Clip::set_loop_range), and tracking is re-engaged when
  /// the result is the default range (0, 0, length). Used by @ref redo() and
  /// by ClipOperator's live drag preview so the committed state always matches
  /// the preview.
  static void applyToClip (
    structure::arrangement::Clip &clip,
    dsp::ContentTick              clip_start,
    dsp::ContentTick              loop_start,
    dsp::ContentTick              loop_end);

  /// Read a snapshot of @p clip's current loop-points state.
  static Snapshot snapshotOf (const structure::arrangement::Clip &clip);

private:
  utils::TypedUuidReference<structure::arrangement::Clip> clip_;
  Snapshot                                                before_;
  Snapshot                                                after_;
};

} // namespace zrythm::commands
