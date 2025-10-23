// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/debug.h"
#include "utils/units.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::structure::arrangement
{
class ArrangerObject;
class MidiNote;
class MidiRegion;
class AudioRegion;
class AutomationRegion;
class ChordRegion;
class ChordObject;
class ScaleObject;
class AutomationPoint;
class Marker;
class AudioSourceObject;

template <typename T>
concept RegionObject =
  std::is_same_v<T, AudioRegion> || std::is_same_v<T, MidiRegion>
  || std::is_same_v<T, AutomationRegion> || std::is_same_v<T, ChordRegion>;

template <typename T>
concept TimelineObject =
  RegionObject<T> || std::is_same_v<T, ScaleObject> || std::is_same_v<T, Marker>;

template <typename T>
concept LaneOwnedObject =
  std::is_same_v<T, MidiRegion> || std::is_same_v<T, AudioRegion>;

template <typename T>
concept FadeableObject = std::is_same_v<T, AudioRegion>;

template <typename T>
concept NamedObject = RegionObject<T> || std::is_same_v<T, Marker>;

template <typename T>
concept BoundedObject = RegionObject<T> || std::is_same_v<T, MidiNote>;

using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  AutomationPoint,
  Marker,
  AudioSourceObject>;
using ArrangerObjectPtrVariant = to_pointer_variant<ArrangerObjectVariant>;

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
  const auto object_position_frames =
    units::samples (obj.position ()->samples ());
  if constexpr (RegionObject<ObjectT>)
    {
      if (normalize)
        {
          auto diff_frames = timeline_frames - object_position_frames;

          /* special case: timeline frames is exactly at the end of the region */
          if (timeline_frames == obj.bounds ()->get_end_position_samples (true))
            return diff_frames;

          const auto loop_end_frames =
            units::samples (obj.loopRange ()->loopEndPosition ()->samples ());
          const auto clip_start_frames =
            units::samples (obj.loopRange ()->clipStartPosition ()->samples ());
          const auto loop_size = obj.loopRange ()->get_loop_length_in_frames ();
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

template <BoundedObject ObjectT>
inline auto *
get_object_bounds (const ObjectT &obj)
{
  return obj.bounds ();
}
} // namespace zrythm::structure::arrangement

DEFINE_UUID_HASH_SPECIALIZATION (
  zrythm::structure::arrangement::ArrangerObjectUuid)
