// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/scale_object.h"

namespace zrythm::structure::arrangement
{

template <FinalArrangerObjectSubclass ObjT>
bool
is_arranger_object_deletable (const ObjT &obj)
{
  if constexpr (std::is_same_v<ObjT, Marker>)
    {
      return obj.markerType () == Marker::MarkerType::Custom;
    }
  else
    {
      return true;
    }
}

/**
 * Returns the number of frames until the next loop end point or the
 * end of the region.
 *
 * @param timeline_frames Global frames at start.
 * @return Number of frames and whether the return frames are for a loop (if
 * false, the return frames are for the region's end).
 */
template <RegionObject ObjectT>
[[gnu::nonnull]] std::pair<signed_frame_t, bool>
get_frames_till_next_loop_or_end (
  const ObjectT &obj,
  signed_frame_t timeline_frames)
{
  const auto * loop_range = obj.loopRange ();
  const auto   loop_size = loop_range->get_loop_length_in_frames ();
  assert (loop_size > 0);
  const auto           object_position_frames = obj.position ()->samples ();
  const signed_frame_t loop_end_frames =
    loop_range->loopEndPosition ()->samples ();
  const signed_frame_t clip_start_frames =
    loop_range->clipStartPosition ()->samples ();

  signed_frame_t local_frames = timeline_frames - object_position_frames;
  local_frames += clip_start_frames;
  while (local_frames >= loop_end_frames)
    {
      local_frames -= loop_size;
    }

  const signed_frame_t frames_till_next_loop = loop_end_frames - local_frames;
  const signed_frame_t frames_till_end =
    obj.bounds ()->get_end_position_samples (true) - timeline_frames;

  return std::make_pair (
    std::min (frames_till_end, frames_till_next_loop),
    frames_till_next_loop < frames_till_end);
}

inline auto
get_object_tick_range (const ArrangerObject * obj)
{
  return std::make_pair (
    obj->position ()->ticks (),
    obj->position ()->ticks ()
      + (obj->bounds () != nullptr ? obj->bounds ()->length ()->ticks () : 0));
}
}
