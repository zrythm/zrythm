// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/debug.h"
#include "utils/units.h"
#include "utils/uuid_identifiable_object.h"

#include <au/math.hh>

namespace zrythm::structure::arrangement
{
class ArrangerObject;
class Clip;
class MidiControlEvent;
class MidiNote;
class MidiClip;
class AudioClip;
class AutomationClip;
class ChordClip;
class ChordObject;
class ScaleObject;
class AutomationPoint;
class Marker;
class AudioSourceObject;
class TempoObject;
class TimeSignatureObject;

template <typename T>
concept ClipObject =
  std::is_same_v<T, AudioClip> || std::is_same_v<T, MidiClip>
  || std::is_same_v<T, AutomationClip> || std::is_same_v<T, ChordClip>;

template <typename T>
concept TimelineObject =
  ClipObject<T> || std::is_same_v<T, ScaleObject> || std::is_same_v<T, Marker>
  || std::is_same_v<T, TempoObject> || std::is_same_v<T, TimeSignatureObject>;

template <typename T>
concept LaneOwnedObject =
  std::is_same_v<T, MidiClip> || std::is_same_v<T, AudioClip>;

template <typename T>
concept FadeableObject = std::is_same_v<T, AudioClip>;

template <typename T>
concept NamedObject = ClipObject<T> || std::is_same_v<T, Marker>;

template <typename T>
concept BoundedObject = ClipObject<T> || std::is_same_v<T, MidiNote>;

template <typename T>
concept EditorObject =
  std::is_same_v<T, MidiControlEvent> || std::is_same_v<T, MidiNote>
  || std::is_same_v<T, AutomationPoint> || std::is_same_v<T, ChordObject>;

using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiClip,
  AudioClip,
  ChordClip,
  AutomationClip,
  AutomationPoint,
  Marker,
  AudioSourceObject,
  TempoObject,
  TimeSignatureObject,
  MidiControlEvent>;
using ArrangerObjectPtrVariant =
  utils::to_pointer_variant<ArrangerObjectVariant>;

using ArrangerObjectUuid = utils::UuidIdentifiableObject<ArrangerObject>::Uuid;

/**
 * Converts frames on the timeline (global) to local frames (in the
 * clip).
 *
 * If @p normalize is true it will only return a position from 0 to loop_end
 * (it will traverse the loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_frames Timeline position in frames.
 *
 * @return The local frames.
 */
template <TimelineObject ObjectT>
[[gnu::hot]] units::sample_t
timeline_frames_to_local (
  const ObjectT  &obj,
  units::sample_t timeline_frames,
  bool            normalize)
{
  const auto &tempo_map = obj.get_tempo_map ();
  const auto  object_position_frames =
    tempo_map.tick_to_samples_rounded (obj.position ()->asTick ());
  if constexpr (ClipObject<ObjectT>)
    {
      if (normalize)
        {
          auto diff_frames = timeline_frames - object_position_frames;

          /* special case: timeline frames is exactly at the end of the clip */
          if (timeline_frames == obj.get_end_position_samples (true))
            return diff_frames;

          const auto * warp = obj.contentWarp ();
          const auto   loop_end_frames =
            warp->contentToTimelineSamples (obj.loopEndPosition ()->asTick ())
            - object_position_frames;
          const auto clip_start_frames =
            warp->contentToTimelineSamples (obj.clipStartPosition ()->asTick ())
            - object_position_frames;
          const auto loop_start_frames =
            warp->contentToTimelineSamples (obj.loopStartPosition ()->asTick ())
            - object_position_frames;
          const auto loop_size =
            max (units::samples (0), loop_end_frames - loop_start_frames);
          assert (loop_size > units::samples (0));

          diff_frames += clip_start_frames;

          while (diff_frames >= loop_end_frames)
            {
              diff_frames -= loop_size;
            }

          return diff_frames;
        }
    }

  return timeline_frames - object_position_frames;
}

} // namespace zrythm::structure::arrangement

DEFINE_UUID_HASH_SPECIALIZATION (
  zrythm::structure::arrangement::ArrangerObjectUuid)
