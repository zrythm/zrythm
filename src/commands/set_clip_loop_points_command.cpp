// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/set_clip_loop_points_command.h"

#include <au/math.hh>

namespace zrythm::commands
{

static constexpr auto k_default_epsilon =
  dsp::ContentTick{ units::ticks (1e-6) };

/// Whether the given positions are the clip's default loop range
/// (0, 0, length) — the range length-tracking reproduces.
static bool
is_default_range (
  const structure::arrangement::Clip &clip,
  dsp::ContentTick                    clip_start,
  dsp::ContentTick                    loop_start,
  dsp::ContentTick                    loop_end)
{
  const auto len = clip.length ()->asTick ();
  return abs (clip_start) < k_default_epsilon && abs (loop_start) < k_default_epsilon
         && abs (loop_end - len) < k_default_epsilon;
}

SetClipLoopPointsCommand::SetClipLoopPointsCommand (
  utils::TypedUuidReference<structure::arrangement::Clip> clip,
  dsp::ContentTick                                        new_clip_start,
  dsp::ContentTick                                        new_loop_start,
  dsp::ContentTick                                        new_loop_end)
    : Command (QObject::tr ("Change Clip Loop Points"), EngineRequirement::None),
      clip_ (std::move (clip))
{
  before_ = snapshotOf (*clip_.get ());
  after_.clip_start = new_clip_start;
  after_.loop_start = new_loop_start;
  after_.loop_end = new_loop_end;
  // redo() derives tracking from the positions (see applyToClip); mirror that
  // derivation here so after_ is a faithful post-redo snapshot.
  after_.track_bounds = is_default_range (
    *clip_.get (), new_clip_start, new_loop_start, new_loop_end);
}

void
SetClipLoopPointsCommand::applyToClip (
  structure::arrangement::Clip &clip,
  dsp::ContentTick              clip_start,
  dsp::ContentTick              loop_start,
  dsp::ContentTick              loop_end)
{
  clip.set_loop_range (clip_start, loop_start, loop_end);

  // Re-engage tracking when the result is the default range (0, 0, length).
  if (is_default_range (clip, clip_start, loop_start, loop_end))
    clip.setTrackBounds (true);
}

SetClipLoopPointsCommand::Snapshot
SetClipLoopPointsCommand::snapshotOf (const structure::arrangement::Clip &clip)
{
  return Snapshot{
    .clip_start = clip.clipStartPosition ()->asTick (),
    .loop_start = clip.loopStartPosition ()->asTick (),
    .loop_end = clip.loopEndPosition ()->asTick (),
    .track_bounds = clip.trackBounds (),
  };
}

void
SetClipLoopPointsCommand::redo ()
{
  auto * clip = clip_.get ();
  if (clip == nullptr)
    return;
  applyToClip (*clip, after_.clip_start, after_.loop_start, after_.loop_end);
}

void
SetClipLoopPointsCommand::undo ()
{
  auto * clip = clip_.get ();
  if (clip == nullptr)
    return;
  // Restore the positions atomically, then re-engage tracking if it was on.
  // set_loop_range disables tracking internally, so setTrackBounds is safe to
  // call afterwards.
  clip->set_loop_range (
    before_.clip_start, before_.loop_start, before_.loop_end);
  if (before_.track_bounds)
    clip->setTrackBounds (true);
}

} // namespace zrythm::commands
