// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_source_object.h"
#include "structure/arrangement/automation_clip.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_control_event.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/scale_object.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/time_signature_object.h"

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

// Currently unused
#if 0
/**
 * Returns the number of frames until the next loop end point or the
 * end of the clip.
 *
 * @param timeline_frames Global frames at start.
 * @return Number of frames and whether the return frames are for a loop (if
 * false, the return frames are for the clip's end).
 */
template <ClipObject ObjectT>
[[gnu::nonnull]] std::pair<int64_t, bool>
get_frames_till_next_loop_or_end (
  const ObjectT &obj,
  int64_t timeline_frames)
{
  const auto   loop_size = obj.get_loop_length_in_frames ();
  assert (loop_size > 0);
  const auto           object_position_frames = obj.position ()->samples ();
  const int64_t loop_end_frames =
    obj.loopEndPosition ()->samples ();
  const int64_t clip_start_frames =
    obj.clipStartPosition ()->samples ();

  int64_t local_frames = timeline_frames - object_position_frames;
  local_frames += clip_start_frames;
  while (local_frames >= loop_end_frames)
    {
      local_frames -= loop_size;
    }

  const int64_t frames_till_next_loop = loop_end_frames - local_frames;
  const int64_t frames_till_end =
    obj.get_end_position_samples (true) - timeline_frames;

  return std::make_pair (
    std::min (frames_till_end, frames_till_next_loop),
    frames_till_next_loop < frames_till_end);
}
#endif

inline auto
get_object_tick_range (const ArrangerObject * obj)
{
  return std::make_pair (
    obj->position ()->ticks (),
    obj->position ()->ticks ()
      + (obj->length () != nullptr ? obj->length ()->ticks () : 0));
}

/**
 * @brief Returns whether the given object is a Clip (MidiClip, AudioClip,
 * ChordClip, or AutomationClip).
 */
inline bool
is_clip (const ArrangerObject &obj)
{
  return qobject_cast<const Clip *> (&obj) != nullptr;
}

/**
 * @brief Returns the absolute timeline position of an arranger object.
 *
 * For timeline objects (clips, markers, etc.): returns the object's own
 * position in timeline ticks.
 *
 * For content objects inside a clip (MidiNotes, AutomationPoints, etc.):
 * converts the content-space position through the parent clip's ContentTimeWarp
 * to get the warp-adjusted absolute timeline position.
 */
inline dsp::TimelineTick
timeline_ticks (const ArrangerObject &obj)
{
  const auto * parent = obj.parentObject ();
  if (parent != nullptr)
    {
      if (const auto * clip = qobject_cast<const Clip *> (parent))
        {
          return clip->contentWarp ()->contentToTimeline (
            qobject_cast<const dsp::ContentPosition *> (obj.position ())
              ->asTick ());
        }
    }
  return qobject_cast<const dsp::TimelinePosition *> (obj.position ())->asTick ();
}

/**
 * @brief Returns the absolute timeline end position of an arranger object.
 *
 * For objects with a length, this is the warp-adjusted end position. For
 * objects without a length (e.g. Marker), returns the same as
 * @ref timeline_ticks.
 */
inline dsp::TimelineTick
timeline_end_ticks (const ArrangerObject &obj)
{
  if (obj.length () == nullptr)
    return timeline_ticks (obj);

  const auto * parent = obj.parentObject ();
  if (parent != nullptr)
    {
      if (const auto * clip = qobject_cast<const Clip *> (parent))
        {
          const auto content_pos =
            qobject_cast<const dsp::ContentPosition *> (obj.position ())
              ->asTick ();
          return clip->contentWarp ()->contentToTimeline (
            content_pos + obj.length ()->asTick ());
        }
    }

  if (const auto * clip = qobject_cast<const Clip *> (&obj))
    {
      return clip->contentWarp ()->contentToTimeline (
        clip->length ()->asTick ());
    }

  // No bounded object can reach here: Bounds implies either Clip (above) or
  // ClipOwned with a parent Clip (handled earlier).
  assert (false);
  return timeline_ticks (obj);
}

/**
 * @brief Sets an object's length so that its timeline end matches the given
 * timeline position.
 *
 * Reverse-warp converts the timeline end into content space before setting
 * the length. No-op for objects without a length or that are not inside a
 * clip / not a clip themselves.
 */
inline void
set_end_from_timeline_ticks (ArrangerObject &obj, dsp::TimelineTick timeline_end)
{
  if (obj.length () == nullptr)
    return;

  // Child object inside a clip — reverse-warp through parent
  const auto * parent = obj.parentObject ();
  if (parent != nullptr)
    {
      if (const auto * parent_clip = qobject_cast<const Clip *> (parent))
        {
          const auto content_end =
            parent_clip->contentWarp ()->timelineToContent (timeline_end);
          const auto content_pos =
            qobject_cast<const dsp::ContentPosition *> (obj.position ())
              ->asTick ();
          obj.length ()->setTicks ((content_end - content_pos).asDouble ());
          return;
        }
    }

  // Clip itself — reverse-warp through own warp
  if (auto * clip = qobject_cast<Clip *> (&obj))
    {
      const auto content_end =
        clip->contentWarp ()->timelineToContent (timeline_end);
      clip->length ()->setTicks (content_end.asDouble ());
    }
}

}
