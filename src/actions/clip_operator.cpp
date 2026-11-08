// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/clip_operator.h"
#include "utils/typed_uuid_reference.h"

namespace zrythm::actions
{

ClipOperator::ClipOperator (
  utils::IObjectRegistry &registry,
  undo::UndoStack        &undoStack,
  QObject *               parent)
    : QObject (parent), registry_ (registry), undo_stack_ (undoStack)
{
}

void
ClipOperator::setClip (structure::arrangement::Clip * clip)
{
  if (clip_ != clip)
    {
      // Finalize an in-flight gesture against the (still-valid) old clip
      // before switching, so no stale command is left referencing it.
      if (drag_active_)
        endClipLoopPointsDrag ();
      clip_ = clip;
      Q_EMIT clipChanged ();
    }
}

void
ClipOperator::beginClipLoopPointsDrag ()
{
  if (clip_ == nullptr)
    return;
  drag_before_ = commands::SetClipLoopPointsCommand::snapshotOf (*clip_);
  drag_active_ = true;
}

void
ClipOperator::updateClipLoopPointsDrag (
  double clipStartTicks,
  double loopStartTicks,
  double loopEndTicks)
{
  if (!drag_active_ || clip_ == nullptr)
    return;
  commands::SetClipLoopPointsCommand::applyToClip (
    *clip_, dsp::ContentTick{ units::ticks (clipStartTicks) },
    dsp::ContentTick{ units::ticks (loopStartTicks) },
    dsp::ContentTick{ units::ticks (loopEndTicks) });
}

void
ClipOperator::endClipLoopPointsDrag ()
{
  if (!drag_active_ || clip_ == nullptr)
    {
      drag_active_ = false;
      return;
    }

  // Snapshot the post-drag state, then restore the clip to the pre-drag state
  // so the command constructor (which captures `before_` from the clip) records
  // the correct origin. The pushed command's redo() will re-apply `after`.
  const auto after = commands::SetClipLoopPointsCommand::snapshotOf (*clip_);
  drag_active_ = false;
  commands::SetClipLoopPointsCommand::applyToClip (
    *clip_, drag_before_.clip_start, drag_before_.loop_start,
    drag_before_.loop_end);

  // No-op gesture: nothing to undo.
  const bool unchanged =
    after.clip_start.asDouble () == drag_before_.clip_start.asDouble ()
    && after.loop_start.asDouble () == drag_before_.loop_start.asDouble ()
    && after.loop_end.asDouble () == drag_before_.loop_end.asDouble ()
    && after.track_bounds == drag_before_.track_bounds;
  if (unchanged)
    return;

  utils::TypedUuidReference<structure::arrangement::Clip> ref{
    clip_->get_uuid (), registry_
  };
  undo_stack_.push (
    std::make_unique<commands::SetClipLoopPointsCommand> (
      std::move (ref), after.clip_start, after.loop_start, after.loop_end));
}

void
ClipOperator::abortClipLoopPointsDrag ()
{
  if (!drag_active_ || clip_ == nullptr)
    {
      drag_active_ = false;
      return;
    }
  // Revert the live preview to the pre-drag state; push nothing.
  commands::SetClipLoopPointsCommand::applyToClip (
    *clip_, drag_before_.clip_start, drag_before_.loop_start,
    drag_before_.loop_end);
  drag_active_ = false;
}

} // namespace zrythm::actions
